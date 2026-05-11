#pragma once
#include <QtCore>
#include <QString>
#include <QByteArray>
#ifndef Q_OS_WIN
#include <unistd.h>
#include <poll.h>
#endif

namespace cpi {

#if QT_VERSION < 0x060000
const auto SkipEmptyParts = QString::SkipEmptyParts;

#else
const auto SkipEmptyParts = Qt::SkipEmptyParts;
const auto flush = Qt::flush;
const auto endl = Qt::endl;
#endif

}


extern std::unique_ptr<QSettings> conf;
extern QStringList cppsArgs;
extern QString aoutName();
extern std::atomic_bool gQuitRequested;
extern void resetTerminalMode();
extern void setTerminalMode(bool enableEcho);
extern QByteArray readStdInput();


#ifndef Q_OS_WIN
extern void Sleep(int msecs);

template <class F>
auto eintr_loop(F&& f)
{
    decltype(f()) ret;
    do {
        errno = 0;
        ret = f();
    } while (ret < 0 && errno == EINTR);
    return ret;
}


inline int eread(int fd, void *buf, size_t len)
{
    return eintr_loop([&] {
        return ::read(fd, buf, len);
    });
}


inline int ewrite(int fd, const void *buf, size_t len)
{
    return eintr_loop([&] {
        return ::write(fd, buf, len);
    });
}


inline int epoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    return eintr_loop([&] {
        return ::poll(fds, nfds, timeout);
    });
}
#endif
