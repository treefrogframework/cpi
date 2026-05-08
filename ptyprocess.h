#pragma once
#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QProcess>

class QSocketNotifier;


class PtyProcess : public QObject
{
    Q_OBJECT
public:
    explicit PtyProcess(QObject *parent = nullptr) :
        QObject(parent)
    {}

    ~PtyProcess() override;
    bool start(const QString &program, const QStringList &arguments);
    qint64 write(const QByteArray &data);
    pid_t pid() const { return _pid; }
    void closeWriteChannel() { }
    bool waitForFinished(int msecs = 30000);
    QProcess::ProcessState state() const { return _state; }
    QByteArray readAll()
    {
        QByteArray result;
        result.swap(_buffer);
        return result;
    }

signals:
    void readyRead();
    void finished(int exitCode);

private slots:
    void readAvailable()
    {
        readFromPty();
        checkFinished();
    }

private:
    void readFromPty();
    bool checkFinished();
    void finishProcess(int exitCode);

private:
    pid_t _pid {-1};
    int _fd {-1};
    QSocketNotifier *_notifier {nullptr};
    QByteArray _buffer;
    QProcess::ProcessState _state {QProcess::NotRunning};
};
