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

extern QSettings *conf;
extern QStringList cppsArgs;
extern QString aoutName();

const QMap<QString, QString> requiredOptions = {
    {"gcc", "-xc"},
    {"g++", "-xc++"},
    {"clang", "-xc"},
    {"clang++", "-xc++"},
    {"cl.exe", "/nologo /EHsc"},
    {"cl", "/nologo /EHsc"},
};


QString Compiler::cxx()
{
    auto compiler = conf->value("CXX").toString().trimmed();

    if (compiler.isEmpty()) {
#if defined(Q_OS_DARWIN)
        compiler = "clang++";
#elif defined(Q_CC_MSVC)
        compiler = "cl.exe";
#else
        auto searchCommand = [](const QString &command) {
            QProcess which;
            which.start("which", QStringList(command));
            which.waitForFinished();
            return which.exitCode() == 0;
        };

        if (searchCommand("g++")) {
            compiler = "g++";
        } else if (searchCommand("clang++")) {
            compiler = "clang++";
        } else {
            qCritical() << "Not found compiler";
            std::exit(1);
        }
#endif
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
    ccOptions << "/Fo" + objtemp.fileName();
    ccOptions << temp.fileName();
#endif

    if (isSetDebugOption()) {
        QFile file("dummy.cpp");
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            file.write(qPrintable(_sourceCode));
            file.close();
        }
    }

    //qDebug() << cc << ccOptions;
    QProcess compileProc;
    compileProc.start(cc, ccOptions);

#ifndef Q_CC_MSVC
    compileProc.write(_sourceCode.toLocal8Bit());
    compileProc.waitForBytesWritten();
    compileProc.closeWriteChannel();
#endif

    compileProc.waitForFinished();
    _compileError = QString::fromLocal8Bit(compileProc.readAllStandardError());

#ifdef Q_CC_MSVC
    objtemp.remove();
    temp.remove();
#endif

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

    QString ccopt = requiredOptions.value(QFileInfo(cc).fileName());
    if (!ccopt.isEmpty()) {
        ccOpts << ccopt;
    }
#ifdef Q_CC_MSVC
    ccOpts << "/Fe:" + aoutName();
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

        auto readfunc = [&]() {
            // read and write to the process
            std::string s;
            if (std::getline(std::cin, s)) {
                QString line = QString::fromStdString(s) + "\n";
                exe.write(line.toLocal8Bit());
            } else {
                exe.closeWriteChannel();
            }
        };

#ifndef Q_OS_WIN
        QSocketNotifier notifier(fileno(stdin), QSocketNotifier::Read);
        QObject::connect(&notifier, &QSocketNotifier::activated, readfunc);
#endif

        while (!exe.waitForFinished(50)) {
            auto exeout = exe.readAll();
            if (!exeout.isEmpty()) {
                print() << exeout << flush;
            }
#ifdef Q_OS_WIN
            HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
            if (WaitForSingleObject(h, 50) == WAIT_OBJECT_0) {
                readfunc();
            }
#endif
            qApp->processEvents();
        }
        print() << exe.readAll() << flush;
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
        print() << "no such file or directory," << path << endl;
        return 1;
    }

    QTextStream ts(&srcFile);
    QString src = ts.readLine().trimmed();  // read first line

    if (src.startsWith("#!")) {
        src = ts.readAll();
    } else {
        src += "\n";
        src += ts.readAll();
    }

    auto opts = cxxflags().split(" ", SkipEmptyParts);
    const QRegularExpression re("//\\s*CompileOptions\\s*:([^\n]*)", QRegularExpression::CaseInsensitiveOption);
    auto match = re.match(src);
    if (match.hasMatch()) {
        opts << match.captured(1).split(" ", SkipEmptyParts);  // compile options
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
        auto errs = _compileError.split("\n");
        if (!errs.value(0).contains("int main()")) {
            printMessage(errs.value(0));
        } else {
            printMessage(errs.value(1));
        }
    }
}
