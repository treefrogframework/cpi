#include "compiler.h"
#include <QtCore/QtCore>

extern QSettings *conf;

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
    return (compile.exitStatus() == QProcess::NormalExit && compile.exitCode() == 0);
}


int Compiler::compileAndExecute(const QString &cc, const QString &ccOptions, const QString &src)
{
    QByteArray aout = (QDir::homePath() + QDir::separator()).toLatin1();

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
    cmd += aout;
    cmd += " - ";  // standard input
    cmd += linkOpts.trimmed();

#if 0
    printf("%s\n", cmd.data());
#endif
    // Mac OS X
    //  cmd = "g++ -pipe -std=c++0x -gdwarf-2 -Wall -W -DQT_CORE_LIB -DQT_SHARED -I/usr/local/Qt4.7/mkspecs/macx-g++ -I. -I/Library/Frameworks/QtCore.framework/Versions/4/Headers -I/usr/include/QtCore -I/usr/include -I. -F/Library/Frameworks -headerpad_max_install_names -F/Library/Frameworks -L/Library/Frameworks -framework QtCore -xc++ -o ";

    // Linux
    // cmd = "g++ -pipe -std=c++0x -D_REENTRANT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4 -L/usr/lib -lQtCore -lpthread -xc++ -o ";

    bool cpl = compile(cmd, qPrintable(src));
    if (cpl) {
        // Executes the binary
        QProcess exe;
        exe.setProcessChannelMode(QProcess::MergedChannels);
        exe.start(aout);
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


bool Compiler::isSetDebugOption()
{
    return QCoreApplication::arguments().contains("-debug");
}


bool Compiler::isSetQtOption()
{
    return QCoreApplication::arguments().contains("-qt");
}


int Compiler::compileFileAndExecute(const QString &path)
{
    QFile srcFile(path);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        qCritical() << "no such file or directory," << path;
        return 1;
    }

    QTextStream ts(&srcFile);
    QString src;
    auto cmd = ts.readLine().trimmed(); // read first line

    if (QFileInfo(srcFile).suffix().toLower() == "cpps") {
        // skip first line
        cmd = ts.readLine().trimmed();
    }

    if (cmd.startsWith("//")) {
        cmd = cmd.mid(2).trimmed();
        src = ts.readAll();
    } else {
        src = cmd + ts.readAll();
        cmd = "gcc -lpthread";
    }
    return compileAndExecute(cmd, src);
}


void Compiler::printLastCompilationError() const
{
    static auto print = [](const QString &msg) {
        int idx = msg.indexOf(": ");
        if (idx > 0) {
            auto s = msg.mid(idx + 1);
            qDebug() << s;
        } else {
            qDebug() << msg;
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
