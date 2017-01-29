#include "compiler.h"
#include <QtCore/QtCore>

extern QSettings *conf;
extern QStringList cppsArgs;

const QMap<QString, QString> requiredOptions = {
    { "gcc",     "-xc" },
    { "g++",     "-xc++" },
    { "clang",   "-xc" },
    { "clang++", "-xc++" },
};


QByteArray Compiler::cc()
{
    return conf->value("CC").toByteArray().trimmed();
}


QByteArray Compiler::cflags()
{
    return conf->value("CFLAGS").toByteArray().trimmed();
}


QByteArray Compiler::ldflags()
{
    return conf->value("LDFLAGS").toByteArray().trimmed();
}


Compiler::Compiler()
{ }


Compiler::~Compiler()
{ }


bool Compiler::compile(const QByteArray &cmd, const QByteArray &code)
{
    _compileError.clear();
    _sourceCode = code.trimmed();

    QProcess compile;
    compile.start(cmd);
    compile.write(_sourceCode);

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
    //qCritical() << _compileError;
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

    QByteArray cmd = cc.toUtf8();
    QByteArray linkOpts;

    for (auto &op : ccOptions.split(" ", QString::SkipEmptyParts)) {
        if (op.startsWith("-L", Qt::CaseInsensitive) || op.startsWith("-Wl,")) {
           linkOpts += " ";
           linkOpts += op.toUtf8();
        } else if (op != "-c") {
           cmd += " ";
           cmd += op.toUtf8();
        }
    }

    QString ccopt = requiredOptions.value(QFileInfo(cc).fileName());
    if (!ccopt.isEmpty()) {
        cmd += " ";
        cmd += ccopt;
    }
    cmd += " -o ";
    cmd += aout.toUtf8();
    cmd += " - ";  // standard input
    cmd += linkOpts.trimmed();
    //qDebug() << cmd;

    bool cpl = compile(cmd, qPrintable(src));
    if (cpl) {
        // Executes the binary
        QString aoutCmd = aout + " " + cppsArgs.join(" ");
        QProcess exe;
        exe.setProcessChannelMode(QProcess::MergedChannels);
        exe.start(aoutCmd);
        exe.waitForFinished();
        printf("%s", exe.readAll().data());
        fflush(stdout);
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

    auto optstr =  cflags() + " " + ldflags();
    auto opt = additionalOptions.value(cc());

    if (!opt.isEmpty()) {
        optstr += " " + opt;
    }
    return compileAndExecute(cc(), optstr, src);
}


int Compiler::compileAndExecute(const QString &cccmd, const QString &src)
{
    auto strs = cccmd.split(" ", QString::SkipEmptyParts);
    QString options = strs.mid(1).join(" ");
    return compileAndExecute(strs.value(0), options, src);
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
        src += ts.readAll();
    }

    const QRegExp re("//\\s*compile:([^\n]*)");
    int pos = re.indexIn(src);
    if (pos >= 0) {
        const auto cmd = re.cap(1); // compile command
        return compileAndExecute(cmd, src);
    }
    return compileAndExecute(src);
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
    qCritical() << _compileError;
}


void Compiler::printContextCompilationError() const
{
    static auto print = [](const QString &msg) {
        int idx = msg.indexOf(": ");
        if (idx > 0) {
            auto s = msg.mid(idx + 1);
            qCritical() << s;
        } else {
            qCritical() << msg;
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


