#include "compiler.h"
#include <QtCore/QtCore>
#include <iostream>

extern QSettings *conf;
extern QStringList cppsArgs;

const QMap<QString, QString> requiredOptions = {
    { "gcc",     "-xc" },
    { "g++",     "-xc++" },
    { "clang",   "-xc" },
    { "clang++", "-xc++" },
};


QString Compiler::cxx()
{
    auto compiler = conf->value("CXX").toString().trimmed();

    if (compiler.isEmpty()) {
#if defined(Q_OS_DARWIN)
        compiler="clang++";
#elif defined(Q_CC_MSVC)
        compiler="cl.exe";
#else
        compiler="g++";
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
{ }


Compiler::~Compiler()
{ }


bool Compiler::compile(const QString &cmd, const QString &code)
{
    _compileError.clear();
    _sourceCode = code.trimmed();

    QProcess compile;
    compile.start(cmd);
    compile.write(_sourceCode.toLocal8Bit());

    if (isSetDebugOption()) {
        QFile file("dummy.cpp");
        if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            file.write(qPrintable(_sourceCode));
            file.close();
        }
    }
    compile.waitForBytesWritten();
    compile.closeWriteChannel();
    compile.waitForFinished();
    _compileError = QString::fromLocal8Bit(compile.readAllStandardError());
    qCritical() << _compileError;
    return (compile.exitStatus() == QProcess::NormalExit && compile.exitCode() == 0);
}


int Compiler::compileAndExecute(const QString &cc, const QString &ccOptions, const QString &src)
{
    QString aout = QDir::homePath() + QDir::separator();

#ifdef Q_OS_WIN32
    aout += ".cpiout.exe";
#else
    aout += ".cpi.out";
#endif

    QString cmd = cc;
    QString linkOpts;

    for (auto &op : ccOptions.split(" ", QString::SkipEmptyParts)) {
        if (op.startsWith("-L", Qt::CaseInsensitive) || op.startsWith("-Wl,")) {
           linkOpts += " ";
           linkOpts += op;
        } else if (op != "-c") {
           cmd += " ";
           cmd += op;
        }
    }

    QString ccopt = requiredOptions.value(QFileInfo(cc).fileName());
    if (!ccopt.isEmpty()) {
        cmd += " ";
        cmd += ccopt;
    }
    cmd += " -o ";
    cmd += aout;
    cmd += " - ";  // standard input
    cmd += linkOpts.trimmed();
    //qDebug() << cmd;

    bool cpl = compile(cmd, qPrintable(src));
    if (cpl) {
        // Executes the binary
        QProcess exe;
        exe.setProcessChannelMode(QProcess::MergedChannels);
        exe.start(aout, cppsArgs);
        exe.waitForStarted();

        while (!exe.waitForFinished(100)) {
            std::cout << exe.readAll().data() << std::flush;
        }
        std::cout << exe.readAll().data() << std::flush;
    }

    QFile::remove(aout);
    return cpl ? 0 : 1;
}


int Compiler::compileAndExecute(const QString &src)
{
    static const QMap<QString, QString> additionalOptions = {
        { "g++",     "-std=c++0x" },
        { "clang++", "-std=c++11" },
    };

    auto optstr = cxxflags() + " " + ldflags();
    auto opt = additionalOptions.value(cxx());

    if (!opt.isEmpty()) {
        optstr += " " + opt;
    }
    return compileAndExecute(cxx(), optstr, src);
}


int Compiler::compileFileAndExecute(const QString &path)
{
    QFile srcFile(path);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        qCritical() << "no such file or directory," << path;
        return 1;
    }

    QTextStream ts(&srcFile);
    QString src = ts.readLine().trimmed(); // read first line

    if (src.startsWith("#!")) {
        src = ts.readAll();
    } else {
        src += "\n";
        src += ts.readAll();
    }

    const QRegExp re("//\\s*CompileOptions\\s*:([^\n]*)");
    int pos = re.indexIn(src);
    if (pos < 0) {
        return compileAndExecute(src);
    }

    auto opts = re.cap(1); // compile options
    const QRegExp reCxx("//\\s*CXX\\s*:([^\n]*)");
    QString cxxCmd;  // compile command
    pos = reCxx.indexIn(src);
    if (pos >= 0) {
        cxxCmd = reCxx.cap(1).trimmed();
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
    std::cout << ">>> Compilation error\n";
    std::cout << qPrintable(_compileError) << std::flush;
}


void Compiler::printContextCompilationError() const
{
    static auto print = [](const QString &msg) {
        int idx = msg.indexOf(": ");
        if (idx > 0) {
            auto s = msg.mid(idx + 1);
            std::cout << qPrintable(s);
        } else {
            std::cout << qPrintable(msg);
        }
    };

    if (_sourceCode.endsWith(';') || _sourceCode.endsWith('}')) {
        // print error
        auto errs = _compileError.split("\n");
        if (!errs.value(0).contains("int main()")) {
            print(errs.value(0));
        } else {
            print(errs.value(1));
        }
    }
}


