#include "ptyprocess.h"
#include <QSocketNotifier>
#include <QElapsedTimer>
#include <QDebug>
#include <iostream>
#include <pty.h>        // forkpty
#include <unistd.h>     // read, execvp, close, _exit
#include <fcntl.h>      // fcntl
#include <sys/wait.h>   // waitpid
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <string.h>


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


PtyProcess::~PtyProcess()
{
    if (_notifier) {
        _notifier->setEnabled(false);
        delete _notifier;
    }

    if (_fd >= 0) {
        ::close(_fd);
    }

    if (_pid > 0) {
        ::kill(_pid, SIGTERM);
        ::waitpid(_pid, nullptr, 0);
    }
}


bool PtyProcess::start(const QString &program, const QStringList &arguments)
{
    if (_pid > 0) {
        qWarning() << "process already started";
        return false;
    }

    int masterFd = -1;
    pid_t pid = ::forkpty(&masterFd, nullptr, nullptr, nullptr);

    if (pid < 0) {
        qWarning() << "forkpty failed:" << strerror(errno);
        return false;
    }

    if (pid == 0) {
        // ---- child process ----
        // forkpty() 時点で stdin/stdout/stderr は slave PTY につなげる
        ::dup2(STDOUT_FILENO, STDERR_FILENO);

        // execvp 用 argv を作る
        QByteArray prog = program.toLocal8Bit();
        std::vector<QByteArray> argBytes;
        argBytes.reserve(arguments.size());

        std::vector<char *> argv;
        argv.reserve(arguments.size() + 2);
        argv.push_back(prog.data());

        for (const QString &arg : arguments) {
            argBytes.push_back(arg.toLocal8Bit());
        }

        for (QByteArray &arg : argBytes) {
            argv.push_back(arg.data());
        }

        argv.push_back(nullptr);
        ::execvp(argv[0], argv.data());

        // execvp が戻るのは失敗時のみ
        std::cerr << "execvp failed" << std::endl;
        return false;
    }

    _pid = pid;
    _fd = masterFd;
    _state = QProcess::Running;

    // non-blocking にする
    int flags = ::fcntl(_fd, F_GETFL, 0);
    if (flags >= 0) {
        ::fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
    }

    _notifier = new QSocketNotifier(_fd, QSocketNotifier::Read, this);
    connect(_notifier, &QSocketNotifier::activated, this, &PtyProcess::readAvailable);
    return true;
}


bool PtyProcess::waitForFinished(int msecs)
{
    if (_state == QProcess::NotRunning) {
        return true;
    }

    QElapsedTimer timer;
    timer.start();

    for (;;) {
        // すでに終了しているか確認
        if (checkFinished()) {
            return true;
        }

        int timeout = -1;

        if (msecs >= 0) {
            qint64 elapsed = timer.elapsed();
            qint64 remain = qint64(msecs) - elapsed;

            if (remain <= 0) {
                return false;
            }

            timeout = int(remain);
        }

        struct pollfd pfd {
            .fd = _fd,
            .events = POLL_IN | POLL_HUP | POLL_ERR,
            .revents = 0
        };

        int r = epoll(&pfd, 1, timeout);

        if (r > 0) {
            if (pfd.revents & (POLL_IN | POLL_HUP | POLL_ERR)) {
                readFromPty();
            }

            if (checkFinished()) {
                return true;
            }

            continue;
        }

        if (r == 0) {
            return false; // timeout
        }

        qWarning() << "poll failed:" << strerror(errno);
        return false;
    }
}


void PtyProcess::readFromPty()
{
    if (_fd < 0) {
        return;
    }

    char buf[4096];

    for (;;) {
        ssize_t n = eread(_fd, buf, sizeof(buf));

        if (n > 0) {
            _buffer.append(buf, int(n));
            emit readyRead();
            continue;
        }

        if (n == 0) {
            checkFinished();
            return;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }

        if (errno == EIO) {
            checkFinished();
            return;
        }

        qWarning() << "read failed:" << strerror(errno);
        checkFinished();
        return;
    }
}


bool PtyProcess::checkFinished()
{
    if (_pid <= 0) {
        return _state == QProcess::NotRunning;
    }

    int status = 0;
    pid_t r = ::waitpid(_pid, &status, WNOHANG);

    if (r == 0) {
        return false;
    }

    if (r < 0) {
        if (errno == EINTR) {
            return false;
        }

        finishProcess(-1);
        return true;
    }

    int exitCode = -1;

    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        exitCode = 128 + WTERMSIG(status);
    }

    finishProcess(exitCode);
    return true;
}


qint64 PtyProcess::write(const QByteArray &data)
{
    if (_fd < 0) {
        return -1;
    }

    const char *p = data.constData();
    qint64 total = 0;
    qint64 remaining = data.size();

    while (remaining > 0) {
        ssize_t n = ewrite(_fd, p + total, size_t(remaining));

        if (n > 0) {
            total += n;
            remaining -= n;
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // non-blocking fd なので、ここで一旦返す
            return total;
        }

        return total > 0 ? total : -1;
    }

    return total;
}


void PtyProcess::finishProcess(int exitCode)
{
    if (_state == QProcess::NotRunning) {
        return;
    }

    if (_notifier) {
        _notifier->setEnabled(false);
        _notifier->deleteLater();
        _notifier = nullptr;
    }

    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }

    _pid = -1;
    _state = QProcess::NotRunning;
    emit finished(exitCode);
}
