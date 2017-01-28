#include <QtCore/QtCore>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <iostream>

#ifdef Q_OS_WIN32
# include <windows.h>
#endif

// #define DEFAULT_CFLAG   "-fPIE -DQT_CORE_LIB "
// #define DEFAULT_LDFLAG  "-lpthread "


static bool compile(const QByteArray &cmd, const QByteArray &code, QByteArray& error)
{
    QProcess compile;
    compile.start(cmd);
    compile.write(code);
#if 0
    QFile file("dummy.cpp");
    if (file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        file.write(qPrintable(code));
        file.close();
    }
#endif
    compile.waitForBytesWritten();
    compile.closeWriteChannel();
    compile.waitForFinished();
    error = compile.readAllStandardError();
    return (compile.exitStatus() == QProcess::NormalExit && compile.exitCode() == 0);
}

static bool compileAndExecute(const QString &cccmd, const QString &src, QByteArray &err)
{
    QByteArray aout = (QDir::homePath() + QDir::separator()).toLatin1();
#ifdef Q_OS_WIN32
    aout += "cpiout.exe";
#else
    aout += "cpi.out";
#endif

    QByteArray cmd;
    QByteArray linkOpts;

    const auto opts = cccmd.split(" ", QString::SkipEmptyParts);
    for (auto &op : opts) {
        if (op.startsWith("-L", Qt::CaseInsensitive) || op.startsWith("-Wl,")) {
            linkOpts += op.toUtf8();
            linkOpts += " ";
        } else if (op != "-c") {
            cmd += op.toUtf8();
            cmd += " ";
        }
    }

    //cmd += "-pipe -xc++ -o ";
    cmd += "-xc++ -o ";
    cmd += aout;
    cmd += " - ";  // standard input
    cmd += linkOpts;

#if 0
    printf("%s\n", cmd.data());
#endif

    std::cout << "compiling " << qPrintable(QCoreApplication::arguments().value(1)) << " ... " << std::flush;
    bool cpl = compile(cmd, qPrintable(src), err);
    std::cout << "done\n";

    if (!cpl) {
        std::cout << ">>> Compilation error\n" <<  err.trimmed().data() << std::endl;
    } else {
        std::cout << "----------------\n";
        // Executes the binary
        QProcess exe;
        QStringList cmdopt = QCoreApplication::arguments().mid(2);
        exe.setProcessChannelMode(QProcess::MergedChannels);
        exe.start(aout, cmdopt);
        exe.waitForStarted();
        QFile::remove(aout);  // remove the file

        while (!exe.waitForFinished(10)) {
            exe.waitForReadyRead(100);
            std::cout << exe.readAll().data();
        }
        std::cout << exe.readAll().data();
    }
    return cpl;
}


int main(int argv, char *argc[])
{
    QCoreApplication app(argv, argc);
    if (argv < 2) {
        return 1;
    }

    QFile srcFile(argc[1]);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        qCritical() << "no such file or directory," << argc[1];
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

    // qDebug() << cmd;
    // qDebug() << src;

    QByteArray err;
    compileAndExecute(cmd, src, err);
    return 0;
}
