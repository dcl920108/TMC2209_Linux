// #include "linux_serial.h"
// #include <termios.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <sys/ioctl.h>
// #include <sys/select.h>

// LinuxSerial::LinuxSerial(const char* port, int /*baud*/) {
//     fd_ = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
//     if (fd_ < 0) return;
//     struct termios tty{};
//     tcgetattr(fd_, &tty);
//     cfsetispeed(&tty, B57600);
//     cfsetospeed(&tty, B57600);
//     tty.c_cflag  = CS8 | CLOCAL | CREAD;
//     tty.c_iflag  = IGNPAR;
//     tty.c_oflag  = 0;
//     tty.c_lflag  = 0;
//     tty.c_cc[VMIN]  = 0;
//     tty.c_cc[VTIME] = 0;
//     tcflush(fd_, TCIOFLUSH);
//     tcsetattr(fd_, TCSANOW, &tty);
// }

// LinuxSerial::~LinuxSerial() {
//     if (fd_ >= 0) close(fd_);
// }

// void LinuxSerial::begin(long /*baud*/) {}

// void LinuxSerial::end() {
//     if (fd_ >= 0) { close(fd_); fd_ = -1; }
// }

// size_t LinuxSerial::write(uint8_t byte) {
//     return write(&byte, 1);
// }

// size_t LinuxSerial::write(const uint8_t* buf, size_t len) {
//     ssize_t n = ::write(fd_, buf, len);
//     tcdrain(fd_);
//     uint8_t echo[64];
//     usleep(len * 200);
//     ::read(fd_, echo, len);
//     return (n > 0) ? n : 0;
// }

// int LinuxSerial::read() {
//     uint8_t buf;
//     fd_set rfds; FD_ZERO(&rfds); FD_SET(fd_, &rfds);
//     struct timeval tv{0, 20000};
//     if (select(fd_ + 1, &rfds, nullptr, nullptr, &tv) <= 0) return -1;
//     return (::read(fd_, &buf, 1) == 1) ? buf : -1;
// }

// int LinuxSerial::available() {
//     int n = 0;
//     ioctl(fd_, FIONREAD, &n);
//     return n;
// }

// void LinuxSerial::flush() {
//     tcdrain(fd_);
// } 4/14/2026

#include "linux_serial.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <cstring>
#include <cstdio>

LinuxSerial::LinuxSerial(const char* port, int /*baud*/) {
    fd_ = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) return;

    struct termios tty;
    tcgetattr(fd_, &tty);

    // Use Linux standard raw mode - matches what pyserial does internally.
    // This is the critical fix: assignment-based flag setup (as in the
    // previous version) silently breaks RX buffer handling on CM4.
    cfmakeraw(&tty);

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    tcflush(fd_, TCIOFLUSH);
    tcsetattr(fd_, TCSANOW, &tty);
    fprintf(stderr, "[UART] opened fd=%d\n", fd_);
}

LinuxSerial::~LinuxSerial() {
    if (fd_ >= 0) close(fd_);
}

// janelia's setup() calls end() then begin(). On Linux, closing the fd here
// would break everything. Both must be no-ops - the port is opened once in
// the constructor and stays open for the object's lifetime.
void LinuxSerial::begin(long /*baud*/) {}
void LinuxSerial::end() {}

// Accumulate bytes into an internal buffer. Actual transmission happens in
// flush(), which janelia calls after a complete datagram is assembled.
// This guarantees inter-byte gaps stay below the datasheet's 63-bit-times
// (547us @ 115200) limit, since the whole datagram is handed to the kernel
// in one ::write() syscall.
size_t LinuxSerial::write(uint8_t byte) {
    if (fd_ < 0) return 0;
    if (tx_len_ < sizeof(tx_buf_)) {
        tx_buf_[tx_len_++] = byte;
    }
    return 1;
}

size_t LinuxSerial::write(const uint8_t* buf, size_t len) {
    if (fd_ < 0) return 0;
    size_t space = sizeof(tx_buf_) - tx_len_;
    size_t to_copy = (len < space) ? len : space;
    memcpy(tx_buf_ + tx_len_, buf, to_copy);
    tx_len_ += to_copy;
    return to_copy;
}

int LinuxSerial::read() {
    if (fd_ < 0) return -1;
    uint8_t buf;
    fd_set rfds; FD_ZERO(&rfds); FD_SET(fd_, &rfds);
    struct timeval tv{0, 170000};  // 170ms timeout - matches PyTmcStepper
    if (select(fd_ + 1, &rfds, nullptr, nullptr, &tv) <= 0) {
        return -1;
    }
    if (::read(fd_, &buf, 1) == 1) {
        return buf;
    }
    return -1;
}

int LinuxSerial::available() {
    if (fd_ < 0) return 0;
    int n = 0;
    ioctl(fd_, FIONREAD, &n);
    return n;
}

// janelia calls flush() after a complete datagram. We use this as the
// trigger to emit the buffered bytes in one ::write() syscall.
void LinuxSerial::flush() {
    if (fd_ < 0 || tx_len_ == 0) return;
    ::write(fd_, tx_buf_, tx_len_);
    tcdrain(fd_);
    tx_len_ = 0;
}
