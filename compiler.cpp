#include "compiler.h"
#include <QtCore/QtCore>

#define DEFAULT_CFLAG   "-fPIE -DQT_CORE_LIB "
#define DEFAULT_LDFLAG  "-lpthread "

extern QSettings *conf;


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
#if 0
    qDebug() << "c====" << _sourceCode;
    qDebug() << "e====" << _compileError;
#endif
    return (compile.exitStatus() == QProcess::NormalExit && compile.exitCode() == 0);
}


int Compiler::compileAndExecute(const QString &src)
{
    QByteArray cc = conf->value("CC").toByteArray();
    QByteArray flags = DEFAULT_CFLAG;
    QByteArray lflags= DEFAULT_LDFLAG;
    flags += conf->value("CC_FLAGS").toByteArray();
    lflags += conf->value("CC_LFLAGS").toByteArray();
    QByteArray aout = (QDir::homePath() + QDir::separator()).toLatin1();
#ifdef Q_OS_WIN32
    aout += "cpiout.exe";
#else
    aout += "cpi.out";
#endif

    QByteArray cmd;
    if (!isSetQtOption()) {
        cmd = cc + " -pipe -std=c++0x " + flags + " -xc++ -o ";
    } else {
#ifdef Q_OS_LINUX
        cmd = cc + " -pipe -std=c++0x -D_REENTRANT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4 -L/usr/lib -lQtCore -lpthread -xc++ -o ";
#endif
    }
    cmd += aout;
    cmd += " - ";  // standard input
    cmd += lflags;
#if 0
    printf("%s\n", cmd.data());
#endif

    // Mac OS X
    //  cmd = "g++ -pipe -std=c++0x -gdwarf-2 -Wall -W -DQT_CORE_LIB -DQT_SHARED -I/usr/local/Qt4.7/mkspecs/macx-g++ -I. -I/Library/Frameworks/QtCore.framework/Versions/4/Headers -I/usr/include/QtCore -I/usr/include -I. -F/Library/Frameworks -headerpad_max_install_names -F/Library/Frameworks -L/Library/Frameworks -framework QtCore -xc++ -o ";

    // Linux
    // cmd = "g++ -pipe -std=c++0x -D_REENTRANT -DQT_NO_DEBUG -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore -I/usr/include/qt4 -L/usr/lib -lQtCore -lpthread -xc++ -o ";

    bool cpl = compile(cmd, qPrintable(src));
    // printf("# %s\n", err.data());
    //printf("----------------\n");
    //qDebug() << _compileError;

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
    return cpl ? 0 : -1;
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
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "file open error");
        return -1;
    }

    QByteArray rawsrc = file.readAll();
    QString src = QString::fromLatin1(rawsrc);

    if (src.mid(0,2) == "#!") {
        // delete first line
        int idx = src.indexOf("\n");
        if (idx > 0)
            src.remove(0, idx);
    }

    QRegExp rx("int\\s+main\\s*\\([^\\(]*\\)");
    if (!src.contains(rx)) {
        QRegExp macro("(#[^\\n]+|\\s*|using\\s+namespace[^\\n]+)\\n");
        int p = 0;
        while (macro.indexIn(src, p) == p) {
            p += macro.matchedLength();
        }
        src.insert(p, "int main(){");
        src += ";return 0;}";
    }

    bool res = compileAndExecute(src);
    if (!res) {
        // print error
        qCritical() << _compileError;
        return -1;
    }
    return 0;
}


static void printCompileError(const QString &msg)
{
    int idx = msg.indexOf(": ");
    if (idx > 0) {
        auto s = msg.mid(idx + 1);
        qDebug() << s;
    } else {
        qDebug() << msg;
    }
}


void Compiler::printLastCompilationError() const
{
    if (_sourceCode.endsWith(';') || _sourceCode.endsWith('}')) {
        // print error
        auto errs = _compileError.split("\n");
        if (!errs.value(0).contains("int main()")) {
            printCompileError(errs.value(0));
        } else {
            printCompileError(errs.value(1));
        }
    }
}
