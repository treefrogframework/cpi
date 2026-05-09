#pragma once
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QDebug>
#include <windows.h>


class PtyProcess : public QObject
{
    Q_OBJECT
public:
    explicit PtyProcess(QObject *parent = nullptr) :
        QObject(parent)
    { }

    ~PtyProcess() override
    {
        kill();
        cleanup();
    }

    bool start(const QString &program, const QStringList &arguments = {})
    {
        if (_state != QProcess::NotRunning) {
            return false;
        }

        setState(QProcess::Starting);

        HANDLE conptyInputRead = nullptr;
        HANDLE conptyInputWrite = nullptr;
        HANDLE conptyOutputRead = nullptr;
        HANDLE conptyOutputWrite = nullptr;
        SECURITY_ATTRIBUTES sa {
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = nullptr,
            .bInheritHandle = FALSE
        };

        // parent write -> ConPTY stdin
        if (!CreatePipe(&conptyInputRead, &conptyInputWrite, &sa, 0)) {
            qWarning() << "CreatePipe input failed:" << GetLastError();
            setState(QProcess::NotRunning);
            return false;
        }

        // ConPTY stdout/stderr -> parent read
        if (!CreatePipe(&conptyOutputRead, &conptyOutputWrite, &sa, 0)) {
            CloseHandle(conptyInputRead);
            CloseHandle(conptyInputWrite);

            qWarning() << "CreatePipe output failed:" << GetLastError();
            setState(QProcess::NotRunning);
            return false;
        }

        COORD size{.X = 120, .Y = 30};
        HRESULT hr = CreatePseudoConsole(size, conptyInputRead, conptyOutputWrite, 0, &_hpc);

        // CreatePseudoConsole 後、親側では不要
        CloseHandle(conptyInputRead);
        CloseHandle(conptyOutputWrite);

        if (FAILED(hr)) {
            CloseHandle(conptyInputWrite);
            CloseHandle(conptyOutputRead);

            qWarning() << "CreatePseudoConsole failed:" << Qt::hex << hr;
            setState(QProcess::NotRunning);
            return false;
        }

        _inputWrite = conptyInputWrite;
        _outputRead = conptyOutputRead;

        STARTUPINFOEXW si {};
        si.StartupInfo.cb = sizeof(si);

        SIZE_T attrSize = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);

        si.lpAttributeList =
            reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
                HeapAlloc(GetProcessHeap(), 0, attrSize)
            );

        if (!si.lpAttributeList) {
            qWarning() << "HeapAlloc failed";
            cleanup();
            setState(QProcess::NotRunning);
            return false;
        }

        if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrSize)) {
            qWarning() << "InitializeProcThreadAttributeList failed:" << GetLastError();

            HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
            cleanup();
            setState(QProcess::NotRunning);
            return false;
        }

        if (!UpdateProcThreadAttribute(
                si.lpAttributeList,
                0,
                PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                _hpc,
                sizeof(_hpc),
                nullptr,
                nullptr)) {
            qWarning() << "UpdateProcThreadAttribute failed:" << GetLastError();

            DeleteProcThreadAttributeList(si.lpAttributeList);
            HeapFree(GetProcessHeap(), 0, si.lpAttributeList);
            cleanup();
            setState(QProcess::NotRunning);
            return false;
        }

        QString commandLine = makeCommandLine(program, arguments);
        std::wstring cmd = commandLine.toStdWString();

        PROCESS_INFORMATION pi {};

        BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, EXTENDED_STARTUPINFO_PRESENT,
            nullptr, nullptr, &si.StartupInfo, &pi);

        DeleteProcThreadAttributeList(si.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, si.lpAttributeList);

        if (!ok) {
            qWarning() << "CreateProcessW failed:" << GetLastError();
            cleanup();
            setState(QProcess::NotRunning);
            return false;
        }

        _process = pi.hProcess;
        _processThread = pi.hThread;
        _running = true;
        _startupFilterEnabled = true;
        _startupFilterTimer.restart();
        _startupVtCarry.clear();

        startReaderThread();
        startWatcherThread();

        setState(QProcess::Running);
        return true;
    }

    QByteArray readAll()
    {
        QMutexLocker locker(&_mutex);

        QByteArray result;
        result.swap(_buffer);
        return result;
    }

    qint64 write(const QByteArray &data)
    {
        if (!_inputWrite || _state == QProcess::NotRunning) {
            return -1;
        }

        DWORD written = 0;
        BOOL ok = WriteFile(_inputWrite, data.constData(), DWORD(data.size()), &written, nullptr);

        if (!ok) {
            return -1;
        }

        return qint64(written);
    }

    bool waitForFinished(int msecs = 30000)
    {
        if (_state == QProcess::NotRunning) {
            return true;
        }

        if (!_process) {
            return true;
        }

        DWORD timeout = (msecs < 0) ? INFINITE : DWORD(msecs);
        DWORD r = WaitForSingleObject(_process, timeout);

        if (r == WAIT_TIMEOUT) {
            return false;
        }

        if (r != WAIT_OBJECT_0) {
            qWarning() << "WaitForSingleObject failed:" << GetLastError();
            return false;
        }

        DWORD code = 0;
        int exitCode = -1;

        if (GetExitCodeProcess(_process, &code)) {
            exitCode = int(code);
        }

        finishProcess(exitCode);
        return true;
    }

    QProcess::ProcessState state() const
    {
        return _state;
    }

    void kill()
    {
        if (_process && _state != QProcess::NotRunning) {
            TerminateProcess(_process, 1);
        }
    }

signals:
    void readyRead();
    void finished(int exitCode);
    void stateChanged(QProcess::ProcessState state);

private:
    void startReaderThread()
    {
        _readerThread = QThread::create([this]() {
            readLoop();
        });

        _readerThread->start();
    }

    void startWatcherThread()
    {
        _watcherThread = QThread::create([this]() {
            DWORD waitResult = WaitForSingleObject(_process, INFINITE);
            int exitCode = -1;

            if (waitResult == WAIT_OBJECT_0) {
                DWORD code = 0;
                if (GetExitCodeProcess(_process, &code)) {
                    exitCode = int(code);
                }
            }

            QMetaObject::invokeMethod(this, [this, exitCode]() {
                    finishProcess(exitCode);
                },
                Qt::QueuedConnection
            );
        });

        _watcherThread->start();
    }

    void readLoop()
    {
        char buf[4096];

        while (_running) {
            DWORD n = 0;
            BOOL ok = ReadFile(_outputRead, buf, sizeof(buf), &n, nullptr);

            if (!ok || n == 0) {
                break;
            }

            QByteArray raw(buf, int(n));
            QByteArray data;

            if (_startupFilterEnabled) {
                data = filterStartupClearSequences(raw);

                if (_startupFilterTimer.elapsed() > 1000) {
                    _startupFilterEnabled = false;

                    if (!_startupVtCarry.isEmpty()) {
                        data += _startupVtCarry;
                        _startupVtCarry.clear();
                    }
                }
            } else {
                data = raw;
            }

            if (!data.isEmpty()) {
                {
                    QMutexLocker locker(&_mutex);
                    _buffer.append(data);
                }

                emit readyRead();
            }
        }
    }

    QByteArray filterStartupClearSequences(const QByteArray &input)
    {
        QByteArray data = _startupVtCarry + input;
        _startupVtCarry.clear();

        QByteArray out;
        out.reserve(data.size());

        int i = 0;

        while (i < data.size()) {
            unsigned char ch = static_cast<unsigned char>(data[i]);

            if (ch != 0x1b) {
                out.append(data[i]);
                ++i;
                continue;
            }

            // If ESC is split at the end of the chunk
            if (i + 1 >= data.size()) {
                _startupVtCarry = data.mid(i);
                break;
            }

            // CSI: ESC [
            if (data[i + 1] == '[') {
                int j = i + 2;

                while (j < data.size()) {
                    unsigned char c = static_cast<unsigned char>(data[j]);

                    // CSI final byte: 0x40 - 0x7e
                    if (c >= 0x40 && c <= 0x7e) {
                        break;
                    }
                    ++j;
                }

                // If the CSI is divided at the chunk boundary
                if (j >= data.size()) {
                    _startupVtCarry = data.mid(i);
                    break;
                }

                QByteArray seq = data.mid(i, j - i + 1);

                if (isStartupClearSequence(seq)) {
                    i = j + 1;
                    continue;
                }

                out.append(seq);
                i = j + 1;
                continue;
            }

            // ESC sequences other than CSI will pass through as is
            out.append(data[i]);
            ++i;
        }

        return out;
    }

    static bool isStartupClearSequence(const QByteArray &seq)
    {
        // clear screen
        if (seq == "\x1b[2J") {
            return true;
        }

        // clear scrollback + screen
        if (seq == "\x1b[3J") {
            return true;
        }

        // cursor home
        if (seq == "\x1b[H" || seq == "\x1b[1;1H") {
            return true;
        }

        // clear line
        if (seq == "\x1b[2K") {
            return true;
        }

        // alternate screen
        if (seq == "\x1b[?1049h" || seq == "\x1b[?1049l") {
            return true;
        }

        if (seq == "\x1b[?1047h" || seq == "\x1b[?1047l") {
            return true;
        }

        if (seq == "\x1b[?47h" || seq == "\x1b[?47l") {
            return true;
        }

        return false;
    }

    void finishProcess(int exitCode)
    {
        if (_state == QProcess::NotRunning) {
            return;
        }

        _running = false;

        if (_inputWrite) {
            CloseHandle(_inputWrite);
            _inputWrite = nullptr;
        }

        if (_hpc) {
            ClosePseudoConsole(_hpc);
            _hpc = nullptr;
        }

        if (_readerThread) {
            if (QThread::currentThread() != _readerThread) {
                _readerThread->wait(1000);
            }

            delete _readerThread;
            _readerThread = nullptr;
        }

        if (_outputRead) {
            CloseHandle(_outputRead);
            _outputRead = nullptr;
        }

        if (_watcherThread) {
            if (QThread::currentThread() != _watcherThread) {
                _watcherThread->wait(1000);
            }

            delete _watcherThread;
            _watcherThread = nullptr;
        }

        if (_processThread) {
            CloseHandle(_processThread);
            _processThread = nullptr;
        }

        if (_process) {
            CloseHandle(_process);
            _process = nullptr;
        }

        setState(QProcess::NotRunning);
        emit finished(exitCode);
    }

    void cleanup()
    {
        _running = false;

        if (_inputWrite) {
            CloseHandle(_inputWrite);
            _inputWrite = nullptr;
        }

        if (_hpc) {
            ClosePseudoConsole(_hpc);
            _hpc = nullptr;
        }

        if (_outputRead) {
            CloseHandle(_outputRead);
            _outputRead = nullptr;
        }

        if (_readerThread) {
            if (QThread::currentThread() != _readerThread) {
                _readerThread->wait(1000);
            }

            delete _readerThread;
            _readerThread = nullptr;
        }

        if (_watcherThread) {
            if (QThread::currentThread() != _watcherThread) {
                _watcherThread->wait(1000);
            }

            delete _watcherThread;
            _watcherThread = nullptr;
        }

        if (_processThread) {
            CloseHandle(_processThread);
            _processThread = nullptr;
        }

        if (_process) {
            CloseHandle(_process);
            _process = nullptr;
        }

        _state = QProcess::NotRunning;
    }

    void setState(QProcess::ProcessState s)
    {
        if (_state == s) {
            return;
        }

        _state = s;
        emit stateChanged(_state);
    }

    static QString quoteArg(const QString &arg)
    {
        if (arg.isEmpty()) {
            return "\"\"";
        }

        const bool needQuote = arg.contains(' ') || arg.contains('\t') || arg.contains('"');

        if (!needQuote) {
            return arg;
        }

        QString r = "\"";
        int backslashes = 0;

        for (QChar ch : arg) {
            if (ch == u'\\') {
                ++backslashes;
            } else if (ch == u'"') {
                r += QString(backslashes * 2 + 1, u'\\');
                r += ch;
                backslashes = 0;
            } else {
                r += QString(backslashes, u'\\');
                r += ch;
                backslashes = 0;
            }
        }

        r += QString(backslashes * 2, u'\\');
        r += "\"";

        return r;
    }

    static QString makeCommandLine(const QString &program, const QStringList &arguments)
    {
        QStringList parts;
        parts << quoteArg(program);

        for (const QString &arg : arguments) {
            parts << quoteArg(arg);
        }

        return parts.join(' ');
    }

private:
    HPCON _hpc {nullptr};
    HANDLE _inputWrite {nullptr};
    HANDLE _outputRead {nullptr};
    HANDLE _process {nullptr};
    HANDLE _processThread {nullptr};
    QThread *_readerThread {nullptr};
    QThread *_watcherThread {nullptr};
    std::atomic_bool _running {false};
    mutable QMutex _mutex;
    QByteArray _buffer;
    bool _startupFilterEnabled {true};
    QElapsedTimer _startupFilterTimer;
    QByteArray _startupVtCarry;
    QProcess::ProcessState _state {QProcess::NotRunning};
};
