#include "global.h"
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <chrono>
#include <thread>

static termios origtio = {};


void resetTerminalMode()
{
    if (origtio.c_lflag) {
        tcsetattr(STDIN_FILENO, TCSANOW, &origtio);
    }
}


void setTerminalMode(bool enableEcho)
{
    if (isatty(STDIN_FILENO)) {
        termios tio {};
        if (tcgetattr(STDIN_FILENO, &tio) == 0) {

            if (!origtio.c_lflag) {
                origtio = tio;
            }

            tio.c_lflag &= ~ICANON;  // disabled canonical mode

            if (enableEcho) {
                tio.c_lflag |= ECHO;  // enabled echo
            } else {
                tio.c_lflag &= ~ECHO;  // disabled echo
            }

            // read one char
            tio.c_cc[VMIN] = 1;
            tio.c_cc[VTIME] = 0;

            if (tcsetattr(STDIN_FILENO, TCSANOW, &tio) != 0) {
                //std::perror("tcsetattr");
            }
        }
    }
}


QByteArray readStdInput()
{
    const int fd = STDIN_FILENO;
    QByteArray bytes;
    char buf[1024];

    pollfd pfd {
        .fd = fd,
        .events = POLLIN,
        .revents = 0
    };

    while (true) {
        int pret = epoll(&pfd, 1, 0);
        if (pret <= 0) {
            break;
        }

        if (!(pfd.revents & POLLIN)) {
            break;
        }

        ssize_t n = eread(fd, buf, sizeof(buf));

        if (n <= 0) {
            break;
        }

        bytes.append(buf, n);
    }

    return bytes;
}


void Sleep(int msecs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
}
