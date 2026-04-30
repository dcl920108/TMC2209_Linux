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

    // Use Linux standard raw mode - matches what pyserial does internally
    cfmakeraw(&tty);

    // Set baud rate
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Ensure receiver enabled and ignore modem lines
    tty.c_cflag |= (CLOCAL | CREAD);

    // Non-blocking read (select handles timeout in read())
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    tcflush(fd_, TCIOFLUSH);
    tcsetattr(fd_, TCSANOW, &tty);
    fprintf(stderr, "[UART] opened fd=%d\n", fd_);
}

LinuxSerial::~LinuxSerial() {
    if (fd_ >= 0) close(fd_);
}

void LinuxSerial::begin(long /*baud*/) {}
void LinuxSerial::end() {}

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
    struct timeval tv{0, 170000};
    if (select(fd_ + 1, &rfds, nullptr, nullptr, &tv) <= 0) {
        fprintf(stderr, "[UART] RX timeout\n");
        return -1;
    }
    if (::read(fd_, &buf, 1) == 1) {
        fprintf(stderr, "[UART] RX %02X\n", buf);
        return buf;
    }
    return -1;
}

int LinuxSerial::available() {
    if (fd_ < 0) return 0;
    int n = 0;
    ioctl(fd_, FIONREAD, &n);
    fprintf(stderr, "[UART] available=%d\n", n);
    return n;
}

void LinuxSerial::flush() {
    if (fd_ < 0 || tx_len_ == 0) return;
    fprintf(stderr, "[UART] flush: sending %zu bytes: ", tx_len_);
    for (size_t i = 0; i < tx_len_; i++) fprintf(stderr, "%02X ", tx_buf_[i]);
    fprintf(stderr, "\n");
    ::write(fd_, tx_buf_, tx_len_);
    tcdrain(fd_);
    tx_len_ = 0;
}
