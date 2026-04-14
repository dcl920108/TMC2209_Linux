# Porting janelia-arduino/TMC2209 to Linux (Raspberry Pi CM4)

This document captures the non-obvious issues I hit while porting
janelia-arduino/TMC2209 to Linux, and how I solved them. If you're attempting
a similar port, read this first — it'll save you a day of debugging.

## The Core Problem

janelia-arduino/TMC2209 is written for Arduino's `HardwareSerial`. To run it
on Linux without modifying the library, you need a shim `HardwareSerial` base
class and a `LinuxSerial` derived class that implements the Arduino serial
interface using Linux `termios`.

The challenge is that several Arduino assumptions don't hold on Linux, and
the failure modes are silent and misleading.

## Issue 1: termios Configuration — THE BIG ONE

This is the issue that cost me most of a day.

### Symptom

- TX works: janelia sends bytes, you can see them on the line
- Echo is received: single-wire UART loops back correctly
- **But the chip's reply never makes it to user space**
- `available()` stays at 0 forever
- janelia's `while (serialAvailable() < WRITE_READ_REPLY_DATAGRAM_SIZE)` loop
  times out every time
- `getVersion()` returns `0x00` instead of `0x21`

### Wrong Approach (What I Did First)

```cpp
tcgetattr(fd_, &tty);
tty.c_cflag  = CS8 | CLOCAL | CREAD;
tty.c_iflag  = IGNPAR;
tty.c_oflag  = 0;
tty.c_lflag  = 0;
tcsetattr(fd_, TCSANOW, &tty);
```

This looks minimal and reasonable, but using **assignment** (`=`) wipes out
system defaults returned by `tcgetattr()`. Some of those defaults interact
with RX buffer handling in ways that silently drop chip replies.

### Correct Approach

Use Linux's standard `cfmakeraw()`, which is equivalent to the raw-mode
template from the termios man page:

```cpp
struct termios tty;
tcgetattr(fd_, &tty);

cfmakeraw(&tty);   // Linux standard raw mode — same thing pyserial does

cfsetispeed(&tty, B115200);
cfsetospeed(&tty, B115200);

tty.c_cflag |= (CLOCAL | CREAD);
tty.c_cc[VMIN]  = 0;
tty.c_cc[VTIME] = 0;

tcflush(fd_, TCIOFLUSH);
tcsetattr(fd_, TCSANOW, &tty);
```

`cfmakeraw()` is equivalent to:

```cpp
c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
c_oflag &= ~OPOST;
c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
c_cflag &= ~(CSIZE | PARENB);
c_cflag |= CS8;
```

The key insight: **use bitwise clear-then-set operations, not assignment.**
Python's pyserial does this internally, which is why pyserial "just works"
while a hand-rolled termios config silently fails.

## Issue 2: `begin()` and `end()` Must Be No-Ops

janelia's `TMC2209::setup()` internally calls:
```cpp
hardware_serial_ptr_->end();
hardware_serial_ptr_->begin(serial_baud_rate);
```

The natural Linux implementation of `end()` is to close the file descriptor.
But then `begin()` (which in Arduino reconfigures UART hardware) has no
equivalent on Linux — the fd is already dead. Everything after `setup()`
fails silently.

**Solution:** Make both functions no-ops. Open the port in the constructor
once, and leave it alone.

```cpp
void LinuxSerial::begin(long /*baud*/) {}
void LinuxSerial::end() {}
```

## Issue 3: Don't Consume Echo Yourself

janelia handles single-wire UART echo removal internally. When you write
`N` bytes, the chip loops them back on RX. janelia's reply-reading path
expects those `N` echo bytes to be in the buffer and consumes them itself.

**If you try to "helpfully" consume the echo in `LinuxSerial::write()`,**
janelia's subsequent read will mistake the chip's reply for echo and
discard it. CRC check fails, and you get no useful data.

**Solution:** Your `write()` implementations should only write. Don't touch
the RX buffer during write operations.

## Issue 4: Inter-Byte Timing on Linux

On Arduino, `HardwareSerial::write(byte)` hands the byte to hardware UART,
which sends it back-to-back with the next byte at the baud rate. Gaps
between bytes are one stop-bit time.

On Linux, each `::write(fd, &byte, 1)` involves a user/kernel context
switch. Bytes *can* end up back-to-back if the kernel's TX buffer is fast
enough, but under load or with scheduling jitter, gaps of hundreds of
microseconds can appear.

**The TMC2209 datasheet specifies that inter-byte gaps over 63 bit-times
(547µs at 115200 baud) invalidate the datagram.** So a byte-by-byte write
loop on Linux is inherently unsafe.

### Solution: Buffer in `write()`, Emit in `flush()`

janelia calls `serialFlush()` after a complete datagram. Use that as your
trigger to emit the whole buffer in one `::write(fd, buf, N)` call:

```cpp
size_t LinuxSerial::write(uint8_t byte) {
    if (tx_len_ < sizeof(tx_buf_)) tx_buf_[tx_len_++] = byte;
    return 1;
}

void LinuxSerial::flush() {
    if (tx_len_ == 0) return;
    ::write(fd_, tx_buf_, tx_len_);
    tcdrain(fd_);
    tx_len_ = 0;
}
```

This guarantees all bytes of a datagram are in one syscall, and the kernel
sends them back-to-back with no user-space gaps.

## Debugging Technique

Instrument every byte in and out from day one. I added `fprintf(stderr, ...)`
at TX, RX, available, and flush:

```cpp
size_t LinuxSerial::write(uint8_t byte) {
    fprintf(stderr, "[UART] TX %02X\n", byte);
    // ...
}

int LinuxSerial::read() {
    // ... after successful read
    fprintf(stderr, "[UART] RX %02X\n", buf);
    return buf;
}

int LinuxSerial::available() {
    int n = 0;
    ioctl(fd_, FIONREAD, &n);
    fprintf(stderr, "[UART] available=%d\n", n);
    return n;
}
```

With this, you can see exactly where communication breaks down. The moment
I saw "echo bytes arriving correctly, but `available()` always 0 for chip
replies," I knew the problem was termios filtering, not wire-level.

## Verification

When the port is working correctly, reading the IOIN register (0x06) via
`getVersion()` should return `0x21`. This is a fixed silicon value — if
you get `0x00`, UART is not working. If you get `0x21`, the whole stack
(termios, janelia, CRC, addressing) is verified end-to-end.

```cpp
TMC2209Driver tmc(...);
tmc.configure(800, 16);
uint8_t v = tmc.getVersion();   // Should be 0x21
```

## Summary of Changes vs Stock janelia

**Zero modifications to janelia source.** All changes live in the
`LinuxSerial` class and a thin wrapper (`TMC2209Driver`) that holds pigpio
GPIO setup and the janelia stepper object.

Files that matter:
- `include/linux_serial.h` — LinuxSerial class declaration
- `src/linux_serial.cpp` — termios setup (use `cfmakeraw`!), buffered write
- `third_party/janelia/` — janelia source, unmodified

## Hardware Tested

- Raspberry Pi CM4 (Cortex-A72, aarch64)
- `/dev/ttyAMA2` via `dtoverlay=uart2` in `/boot/firmware/config.txt`
- TMC2209 with PDN_UART on single-wire mode (1kΩ between TX and RX)
- 115200 baud
- NODEADDR = 0 (MS1/MS2 both grounded)

## Credit

This port was done for an AGD (Alcohol Genotype Device) PCR system where
the existing Python driver (PyTmcStepper) had GPIO cleanup issues across
multiple runs. Having a C++ driver with clean RAII GPIO handling solves
that category of problems.
