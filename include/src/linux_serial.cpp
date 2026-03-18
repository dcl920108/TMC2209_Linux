#include "linux_serial.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

LinuxSerial::LinuxSerial(const char* port, int /*baud*/) {
    fd_ = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) return;
    struct termios tty{};
    tcgetattr(fd_, &tty);
    cfsetispeed(&tty, B57600);
    cfsetospeed(&tty, B57600);
    tty.c_cflag  = CS8 | CLOCAL | CREAD;
    tty.c_iflag  = IGNPAR;
    tty.c_oflag  = 0;
    tty.c_lflag  = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;
    tcflush(fd_, TCIOFLUSH);
    tcsetattr(fd_, TCSANOW, &tty);
}

LinuxSerial::~LinuxSerial() {
    if (fd_ >= 0) close(fd_);
}

void LinuxSerial::begin(long /*baud*/) {}

void LinuxSerial::end() {
    if (fd_ >= 0) { close(fd_); fd_ = -1; }
}

size_t LinuxSerial::write(uint8_t byte) {
    return write(&byte, 1);
}

size_t LinuxSerial::write(const uint8_t* buf, size_t len) {
    ssize_t n = ::write(fd_, buf, len);
    tcdrain(fd_);
    uint8_t echo[64];
    usleep(len * 200);
    ::read(fd_, echo, len);
    return (n > 0) ? n : 0;
}

int LinuxSerial::read() {
    uint8_t buf;
    fd_set rfds; FD_ZERO(&rfds); FD_SET(fd_, &rfds);
    struct timeval tv{0, 20000};
    if (select(fd_ + 1, &rfds, nullptr, nullptr, &tv) <= 0) return -1;
    return (::read(fd_, &buf, 1) == 1) ? buf : -1;
}

int LinuxSerial::available() {
    int n = 0;
    ioctl(fd_, FIONREAD, &n);
    return n;
}

void LinuxSerial::flush() {
    tcdrain(fd_);
}
