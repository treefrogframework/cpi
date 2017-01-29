#include <QtCore>
#include "compiler.h"
#include "codegenerator.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef Q_OS_WIN32
# include <windows.h>
#endif

#define DEFAULT_CONFIG                                          \
    "[General]\n"                                               \
    "### Example option for Qt5\n"                              \
    "#CC=g++\n"                                                 \
    "#CFLAGS=-fPIC -pipe -std=c++0x -D_REENTRANT -I/usr/include/qt5\n" \
    "#LDFLAGS=-lQt5Core\n"                                      \
    "#COMMON_INCLUDES=\n"                                       \
    "\n"                                                        \
    "CC=g++\n"                                                  \
    "CFLAGS=-fPIC -pipe -std=c++0x -D_REENTRANT\n"              \
    "LDFLAGS=\n"                                                \
    "COMMON_INCLUDES=\n"


// Entered headers and code
static QStringList headers, code;
QSettings *conf;


static QString isSetFileOption()
{
    QString ret;
    for (int i = 1; i < QCoreApplication::arguments().length(); i++) {
        auto opt = QCoreApplication::arguments()[i];
        if (!opt.startsWith("-")) {
            ret = opt;
            break;
        }
    }
    return ret;
}


static void showHelp()
{
    char help[] =
        " .conf        Display the current values for various settings.\n" \
        " .help        Display this help.\n"                               \
        " .rm LINENO   Remove the code of the specified line number.\n"    \
        " .show        Show the current source code.\n"                    \
        " .quit        Exit this program.\n";
    printf("%s", help);
}


static void showConfigs(const QSettings &conf)
{
    QStringList confkeys;
    confkeys << "CC" << "CC_FLAGS" << "CC_LFLAGS" << "COMMON_INCLUDES";

    QStringList configs = conf.allKeys();
    for (QStringListIterator it(conf.allKeys()); it.hasNext(); ) {
        const QString &key = it.next();
        if (confkeys.contains(key))
            printf("%s=%s\n", qPrintable(key), qPrintable(conf.value(key).toString()));
    }
}


static void deleteLine(int n)
{
    int h = headers.count();
    int c = code.count();

    if (n > 0) {
        if (n <= h) {
            headers.removeAt(n - 1);
        } else if (n <= h + c) {
            code.removeAt(n - h - 1);
        } else {
            // ignore
        }
    }
}


static void deleteLines(const QList<int> &numbers)
{
    QList<int> list = numbers.toSet().toList(); // removes duplicates
    qSort(list.begin(), list.end(), qGreater<int>());  // sort
    for (QListIterator<int> it(list); it.hasNext(); ) {
        deleteLine(it.next());
    }
}


static void showCode()
{
    int num = 1;
    if (!headers.isEmpty()) {
        for (int i = 0; i < headers.count(); ++i)
            printf("%3d| %s\n", num++, qPrintable(headers.at(i)));
        printf("    --------------------\n");
    }
    for (int i = 0; i < code.count(); ++i)
        printf("%3d| %s\n", num++, qPrintable(code.at(i)));
}


static bool waitForReadyReadStdin(int msec)
{
#ifdef Q_OS_WIN32
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    return (WaitForSingleObject(h, msec) == WAIT_OBJECT_0);
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval tv = { 0, 0 };
    tv.tv_usec = 1000 * msec;
    int stdinReady = select(1, &fds, NULL, NULL, &tv); // select for stdin
    if (stdinReady < 0) {
        fprintf(stderr, "select error\n");
    }
    return (stdinReady > 0);
#endif
}


int main(int argv, char *argc[])
{
    QCoreApplication app(argv, argc);

#if (defined Q_OS_WIN32) || (defined Q_OS_DARWIN)
    conf = new QSettings(QSettings::IniFormat, QSettings::UserScope, "cpi/cpi");
#else
    conf = new QSettings(QSettings::NativeFormat, QSettings::UserScope, "cpi/cpi");
#endif

    QFile confFile(conf->fileName());
    if (!confFile.exists()) {
        if (confFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            confFile.write(DEFAULT_CONFIG);
            confFile.close();
        }
        conf->sync();
    }

    Compiler compiler;
    QString file = isSetFileOption();

    if (!file.isEmpty()) {
        int ret = compiler.compileFileAndExecute(file);
        if (ret) {
            compiler.printLastCompilationError();
        }
        return ret;
    }

    printf("Loaded INI file: %s\n", qPrintable(conf->fileName()));

    QStringList includes = conf->value("COMMON_INCLUDES").toString().split(" ", QString::SkipEmptyParts);
    for (QStringListIterator i(includes); i.hasNext(); ) {
        QString s = i.next().trimmed();
        if (!s.isEmpty()) {
            if (s.startsWith("<") || s.startsWith('"')) {
                headers << QString("#include ") + s;
            } else {
                headers << QString("#include <") + s + ">";
            }
        }
    }

    bool stdinReady = false;
    for (;;) {
        char line[1024];

        if (!stdinReady) {
            printf("cpi> ");
        }

        char *res = fgets(line, sizeof(line), stdin);

        QString str = QString(line).trimmed();
        if (!res || str == ".quit" || str == ".q")
            break;

        if (str == ".help" || str == "?") {  // shows help
            showHelp();
            continue;
        }

        if (str == ".show" || str == ".code") {  // shows code
            showCode();
            continue;
        }

        if (str == ".conf") {  // shows configs
            showConfigs(*conf);
            continue;
        }

        if (str.startsWith(".del ") || str.startsWith(".rm ")) { // Deletes code
            int n = str.indexOf(' ');
            str.remove(0, n + 1);
            QStringList list = str.split(QRegExp("[,\\s]"), QString::SkipEmptyParts);

            QList<int> numbers; // line-numbers
            for (QStringListIterator it(list); it.hasNext(); ) {
                const QString &s = it.next();
                bool ok;
                int n = s.toInt(&ok);
                if (ok && n > 0)
                    numbers << n;
            }
            deleteLines(numbers);
            showCode();
            continue;
        }

        if (str.startsWith('#') || str.startsWith("using ")) {
            headers << str;
        } else {
            if (!str.isEmpty()) {
                code << str;
            }
        }

        if (code.isEmpty())
            continue;

        stdinReady = waitForReadyReadStdin(10); // wait for 10msec
        if (stdinReady)
            continue;

        // compile
        CodeGenerator cdgen(headers.join("\n"), code.join("\n"));
        QString src = cdgen.generateMainFunc();

        //QByteArray err;
        int cpl = compiler.compileAndExecute(src);
        if (cpl) {
            // compile only once more
            src = cdgen.generateMainFuncSafe();
            cpl = compiler.compileAndExecute(src);
        }

        if (cpl) {
            compiler.printLastCompilationError();
        }
    }
    delete conf;
    return 0;
}
