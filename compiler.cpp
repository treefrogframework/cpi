#include "compiler.h"
#include "global.h"
#include "print.h"
#include <QtCore/QtCore>
#include <cstdlib>
#include <iostream>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
using namespace cpi;


const QList<QPair<QString, QStringList>> requiredOptions = {
    {"gcc", {"-xc"}},
    {"g++", {"-xc++"}},
    {"clang", {"-xc"}},
    {"clang++", {"-xc++"}},
#if Q_OS_WIN
    {"cl.exe", {"-nologo", "-EHsc"}},
    {"cl", {"-nologo", "-EHsc"}},
#endif
};


static QString searchPath(const QString &command)
{
    QString path;
    QProcess which;

#if defined(Q_OS_WIN)
    which.start("where.exe", QStringList(command));
#else
    which.start("which", QStringList(command));
#endif
    which.waitForFinished();
    if (which.exitCode() == 0) {
        path = QString::fromLocal8Bit(which.readAll()).split("\n", SkipEmptyParts).value(0).trimmed();
    }
    return path;
};


QString Compiler::cxx()
{
    QString compiler = conf->value("CXX").toString().trimmed();

    if (compiler.isEmpty()) {
#if defined(Q_OS_DARWIN)
        compiler = searchPath("clang++");
#elif defined(Q_CC_MSVC)
        compiler = searchPath("cl.exe");
#else
        compiler = searchPath("g++");
        if (compiler.isEmpty()) {
            compiler = searchPath("clang++");
        }
#endif
    } else {
#ifdef Q_OS_WIN
        QFileInfo fi(compiler);
        if (!fi.isAbsolute()) {
            compiler = searchPath(compiler);
        }
#else
        // check path
        compiler = searchPath(compiler);
#endif
    }

    if (compiler.isEmpty()) {
        qCritical() << "Compiler not found." << conf->value("CXX").toString().trimmed();
#ifdef Q_OS_WIN
        qCritical() << "Remember to call vcvarsall.bat to complete environment setup.";
#endif
        std::exit(1);
    }

    return compiler;
}


QString Compiler::cxxflags()
{
    return conf->value("CXXFLAGS").toString().trimmed();
}


QString Compiler::ldflags()
{
    return conf->value("LDFLAGS").toString().trimmed();
}


Compiler::Compiler()
{
}


Compiler::~Compiler()
{
}


bool Compiler::compile(const QString &cc, const QStringList &options, const QString &code)
{
    _compileError.clear();
    _sourceCode = code.trimmed();
    auto ccOptions = options;

#ifdef Q_CC_MSVC
    QFile objtemp(QDir::tempPath() + QDir::separator() + "cpisource" + QString::number(QCoreApplication::applicationPid()) + ".obj");
    QFile temp(QDir::tempPath() + QDir::separator() + "cpisource" + QString::number(QCoreApplication::applicationPid()) + ".cpp");
    if (temp.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        temp.write(qPrintable(_sourceCode));
        temp.close();
    }
    ccOptions << "-Fo" + objtemp.fileName();
    ccOptions << temp.fileName();
#endif

    if (isSetDebugOption()) {
        QFile file("dummy.cpp");
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            file.write(qPrintable(_sourceCode));
            file.close();
        }
    }

    // qDebug() << cc << ccOptions;
    // qDebug() << code;
    QProcess compileProc;
    compileProc.start(cc, ccOptions);

#ifndef Q_CC_MSVC
    compileProc.write(_sourceCode.toLocal8Bit());
    compileProc.waitForBytesWritten();
    compileProc.closeWriteChannel();
#endif
    compileProc.waitForFinished();

#ifdef Q_CC_MSVC
    _compileError = QString::fromLocal8Bit(compileProc.readAllStandardOutput());
    objtemp.remove();
    temp.remove();
#else
    _compileError = QString::fromLocal8Bit(compileProc.readAllStandardError());
#endif
    // qDebug() << "#" << _compileError << "#";

    return (compileProc.exitStatus() == QProcess::NormalExit && compileProc.exitCode() == 0);
}


int Compiler::compileAndExecute(const QString &cc, const QStringList &options, const QString &src)
{
    QStringList ccOpts;
    QStringList linkOpts;

    for (auto &op : options) {
#ifndef Q_CC_MSVC
        if (op.startsWith("-L", Qt::CaseInsensitive) || op.startsWith("-Wl,")) {
            linkOpts << op;
            continue;
        }
#endif
        if (op != "-c") {
            ccOpts << op;
            continue;
        }
    }

    QStringList ccopt;
    QString fname = QFileInfo(cc).fileName();
    for (const auto &it : requiredOptions) {
        if (fname.startsWith(it.first)) {
            ccOpts << it.second;
            break;
        }
    }

#ifdef Q_CC_MSVC
    ccOpts << "-Fe:" + aoutName();
#else
    ccOpts << "-o";
    ccOpts << aoutName();
    ccOpts << "-";  // standard input
#endif
    ccOpts << linkOpts;

    bool cpl = compile(cc, ccOpts, src);
    if (cpl) {
        // Executes the binary
        QProcess exe;
        exe.setProcessChannelMode(QProcess::MergedChannels);
        exe.start(aoutName(), cppsArgs);
        exe.waitForStarted();

        auto readStdInput = [&]() {
            // read and write to the process
            std::string s;
            if (std::getline(std::cin, s)) {
                auto line = QByteArray::fromStdString(s);
                line += "\n";
                exe.write(line.data());
            } else {
                exe.closeWriteChannel();
            }
        };

#ifndef Q_OS_WIN
        QSocketNotifier notifier(fileno(stdin), QSocketNotifier::Read);
        QObject::connect(&notifier, &QSocketNotifier::activated, readStdInput);
#endif

        while (!exe.waitForFinished(50)) {
            auto exeout = exe.readAll();
            if (!exeout.isEmpty()) {
                // stdout raw data
                std::cout << exeout.data() << std::flush;
            }

#ifdef Q_OS_WIN
            HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
            if (WaitForSingleObject(h, 50) == WAIT_OBJECT_0) {
                readStdInput();
            }
#endif
            if (exe.state() != QProcess::Running) {
                break;
            }
            qApp->processEvents();
        }
        // stdout raw data
        std::cout << exe.readAll().data() << std::flush;
    }

    QFile::remove(aoutName());
    return cpl ? 0 : 1;
}


int Compiler::compileAndExecute(const QString &src)
{
    auto opts = cxxflags().split(" ", SkipEmptyParts);
    opts << ldflags().split(" ", SkipEmptyParts);
    return compileAndExecute(cxx(), opts, src);
}


int Compiler::compileFileAndExecute(const QString &path)
{
    QFile srcFile(path);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        print() << "File open error, " << path << endl;
        return 1;
    }

    QTextStream ts(&srcFile);
#if QT_VERSION >= 0x060000
    ts.setEncoding(QStringConverter::System);
#endif
    QString src = ts.readLine().trimmed();  // read first line

    if (src.startsWith("#!")) {  // check shebang
        src = ts.readAll();
    } else {
        src += "\n";
        src += ts.readAll();
    }

    auto opts = cxxflags().split(" ", SkipEmptyParts);
    const QRegularExpression re("//\\s*CompileOptions\\s*:([^\n]*)", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(src);

    if (match.hasMatch()) {
        // Command substitution
        QString options = match.captured(1);
        QStringList subsList { "`([^`]+)`", "\\$\\(([^\\)]+)\\)" };

        for (const auto &subs : subsList) {
            QRegularExpression recs(subs);
            auto matchcs = recs.match(options);
            while (matchcs.hasMatch()) {
                int pos = matchcs.capturedStart(0);
                int len = matchcs.capturedLength(0);
                options = options.remove(pos, len);

                auto cmd = matchcs.captured(1).split(" ", SkipEmptyParts);  // Matched text
                if (!cmd.isEmpty()) {
                    QProcess shproc;
                    auto cmdpath = searchPath(cmd[0]);
                    if (cmdpath.isEmpty()) {
                        cmdpath = cmd[0];
                    }

                    shproc.start(cmdpath, cmd.mid(1));
                    shproc.waitForFinished();

                    if (shproc.exitCode() == 0) {
                        QByteArray out = shproc.readAllStandardOutput().trimmed();
                        options.insert(pos, QString::fromLocal8Bit(out));
                    }
                }

                // Retry matching
                matchcs = recs.match(options);
            }
        }
        //qDebug() << "CompileOptions: " << options;
        opts << options.split(" ", SkipEmptyParts);  // compile options
    }

    const QRegularExpression reCxx("//\\s*CXX\\s*:([^\n]*)");
    auto cxxMatch = reCxx.match(src);
    QString cxxCmd;  // compile command
    if (cxxMatch.hasMatch()) {
        cxxCmd = cxxMatch.captured(1).trimmed();
    }

    if (cxxCmd.isEmpty()) {
        cxxCmd = cxx();  // cxx command
    }

    return compileAndExecute(cxxCmd, opts, src);
}


bool Compiler::isSetDebugOption()
{
    return QCoreApplication::arguments().contains("-debug");
}


bool Compiler::isSetQtOption()
{
    return QCoreApplication::arguments().contains("-qt");
}


void Compiler::printLastCompilationError() const
{
    print() << ">>> Compilation error\n";
    print() << _compileError << flush;
}


void Compiler::printContextCompilationError() const
{
    static auto printMessage = [](const QString &msg) {
        // output after the first colon
        int idx = msg.indexOf(": ");
        if (idx > 0) {
            auto s = msg.mid(idx + 1);
            print() << s << endl;
        } else {
            print() << msg << endl;
        }
    };

    if (_sourceCode.endsWith(';') || _sourceCode.endsWith('}')) {
        // print error
#ifdef Q_CC_MSVC
        printMessage(_compileError);
#else
        auto errs = _compileError.split("\n");
        if (!errs.value(0).contains("int main()")) {
            printMessage(errs.value(0) + errs.value(1));
        } else {
            printMessage(errs.value(1) + errs.value(2));
        }
#endif
    }
}
