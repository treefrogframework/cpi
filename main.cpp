#include "codegenerator.h"
#include "compiler.h"
#include "global.h"
#include "print.h"
#include <QtCore/QtCore>
#include <cstdlib>
#include <iostream>
#include <list>
#ifdef Q_OS_WIN
#include <conio.h>
#include <windows.h>
#else
#include <csignal>
#include <unistd.h>
#endif
using namespace cpi;

// Version
constexpr auto CPI_VERSION_STR = "2.1.0";

#ifdef Q_CC_MSVC
constexpr auto DEFAULT_CONFIG = "[General]\n"
                                "CXX=cl.exe\n"
                                "CXXFLAGS=-std:c++20\n"
                                "LDFLAGS=\n"
                                "COMMON_INCLUDES=\n";
#else
#if QT_VERSION < 0x060000
constexpr auto DEFAULT_CONFIG = "[General]\n"
                                "### Example option for Qt5\n"
                                "#CXX=%1\n"
                                "#CXXFLAGS=-pipe -std=c++14 -D_REENTRANT -I/usr/include/qt5\n"
                                "#LDFLAGS=-lQt5Core\n"
                                "#COMMON_INCLUDES=\n"
                                "\n"
                                "CXX=\n"
                                "CXXFLAGS=-pipe -std=c++14 -D_REENTRANT\n"
                                "LDFLAGS=\n"
                                "COMMON_INCLUDES=\n";
#else
constexpr auto DEFAULT_CONFIG = "[General]\n"
                                "### Example option for Qt6\n"
                                "#CXX=\n"
                                "#CXXFLAGS=-pipe -std=c++2a -D_REENTRANT -I/usr/include/qt6\n"
                                "#LDFLAGS=-lQt6Core\n"
                                "#COMMON_INCLUDES=\n"
                                "\n"
                                "CXX=\n"
                                "CXXFLAGS=-pipe -std=c++2a -D_REENTRANT\n"
                                "LDFLAGS=\n"
                                "COMMON_INCLUDES=\n";
#endif
#endif

// Entered headers and code
static QStringList headers, code;
static int lastLineNumber = 0;  // line number added recently
std::unique_ptr<QSettings> conf;
QStringList cppsArgs;


QString aoutName()
{
    static QString aout;
    if (aout.isEmpty()) {
        aout = QDir::tempPath() + QDir::separator();
#ifdef Q_OS_WIN
        aout += ".cpiout" + QString::number(QCoreApplication::applicationPid()) + ".exe";
#else
        aout += ".cpi" + QString::number(QCoreApplication::applicationPid()) + ".out";
#endif
    }
    return aout;
}


#ifdef Q_OS_WIN
static BOOL WINAPI signalHandler(DWORD ctrlType)
{
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT: {
        // cleanup
        if (QFileInfo(aoutName()).exists()) {
            QFile::remove(aoutName());
        }
        break;
    }

    default:
        return FALSE;
    }

    while (true)
        Sleep(1);

    return TRUE;
}
#else

static void signalHandler(int)
{
    // cleanup
    if (QFileInfo(aoutName()).exists()) {
        QFile::remove(aoutName());
    }
    std::exit(0);
}

static void watchUnixSignal(int sig)
{
    if (sig < NSIG) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = SA_RESTART;
        sa.sa_handler = signalHandler;
        sigaction(sig, &sa, 0);
    }
}
#endif


static QString isSetFileOption()
{
    QString ret;
    for (int i = 1; i < QCoreApplication::arguments().length(); i++) {
        auto opt = QCoreApplication::arguments()[i];
        if (!opt.startsWith("-") && QFileInfo(opt).exists()) {
            ret = opt;
            cppsArgs = QCoreApplication::arguments().mid(i + 1);
            break;
        }
    }
    return ret;
}


static void showHelp()
{
    char help[] = " .conf        Display the current values for various settings.\n"
                  " .help        Display this help.\n"
                  " .rm LINENO   Remove the code of the specified line number.\n"
                  " .clear       Clear the code all.\n"
                  " .show        Show the current source code.\n"
                  " .quit        Exit this program.\n";
    print() << help;
}


static void showConfigs(const QSettings &conf)
{
    const QStringList confkeys {"CXX", "CXXFLAGS", "LDFLAGS", "COMMON_INCLUDES"};

    for (auto &key : conf.allKeys()) {
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
        lastLineNumber = 0;
    }
}


static void deleteLines(const std::list<int> &numbers)
{
    std::list<int> numlist = numbers;
    numlist.sort(std::greater<int>());
    numlist.unique();  // removes duplicates

    for (auto d : numlist) {
        deleteLine(d);
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


static bool waitForReadyStdInputRead(int msecs)
{
    QElapsedTimer timer;
    timer.start();

#ifdef Q_OS_WIN
    while (timer.elapsed() < msecs) {
        if (_kbhit()) {
            return true;
        }
        QThread::msleep(20);
    }
    return false;
#else
    bool ret = false;
    QSocketNotifier notifier(fileno(stdin), QSocketNotifier::Read);
    QObject::connect(&notifier, &QSocketNotifier::activated, [&](){ ret = true; });

    for (;;) {
        qApp->processEvents(QEventLoop::AllEvents);
        if (timer.elapsed() >= msecs || ret) {
            break;
        }
        QThread::msleep(20);
    }
    return ret;
#endif
}


static void compile()
{
    if (code.isEmpty()) {
        return;
    }

    // compile
    CodeGenerator cdgen(headers.join("\n"), code.join("\n"));
    QString src = cdgen.generateMainFunc();

    Compiler compiler;
    int cpl = compiler.compileAndExecute(src);
    if (cpl) {
        // compile only once more
        src = cdgen.generateMainFunc(true);
        cpl = compiler.compileAndExecute(src);
    }

    if (cpl) {
        compiler.printContextCompilationError();
        if (!code.join("\n").contains(QRegularExpression(" main\\s*\\("))) {
            // delete last line
            if (lastLineNumber > 0) {
                deleteLine(lastLineNumber);
            }
        }
    }
};


static QString readLine()
{
    QString line;
    std::string s;

    if (std::getline(std::cin, s)) {
        // Countermeasure for garbled characters in windows
        line = QString::fromLocal8Bit(QByteArray::fromStdString(s));
    }
    return line;
}


static int interpreter()
{
    print() << "cpi " << CPI_VERSION_STR << endl;
    print() << "Type \".help\" for more information." << endl;
    print() << "Loaded INI file: " << conf->fileName() << endl;
    print().flush();

    const QStringList includes = conf->value("COMMON_INCLUDES").toString().split(" ", SkipEmptyParts);
    for (auto s : includes) {
        s = s.trimmed();
        if (!s.isEmpty()) {
            if (s.startsWith("<") || s.startsWith('"')) {
                headers << QString("#include ") + s;
            } else {
                headers << QString("#include <") + s + ">";
            }
            lastLineNumber = headers.count();
        }
    }

    bool end = false;
    auto readCodeAndCompile = [&]() {
        QString line = readLine();

        if (line.isNull()) {
            end = true;
            return;
        }

        QString cmd = line.trimmed();
        if (cmd == ".quit" || cmd == ".q") {
            end = true;
            return;
        }

        class PromptOut {
        public:
            ~PromptOut() { print() << prompt << flush; }
            void off() { prompt.clear(); }
        private:
            QByteArray prompt {"cpi> "};
        } promptOut;

        if (cmd == ".help" || cmd == "?") {  // shows help
            showHelp();
            return;
        }

        if (cmd == ".show" || cmd == ".code") {  // shows code
            showCode();
            return;
        }

        if (cmd == ".conf") {  // shows configs
            showConfigs(*conf);
            return;
        }

        if (cmd.startsWith(".del ") || cmd.startsWith(".rm ")) {  // Deletes code
            int n = cmd.indexOf(' ');
            cmd.remove(0, n + 1);
            const QStringList list = cmd.split(QRegularExpression("[,\\s]"), SkipEmptyParts);

            std::list<int> numbers;  // line-numbers
            for (auto &s : list) {
                bool ok;
                int n = s.toInt(&ok);
                if (ok && n > 0)
                    numbers.push_back(n);
            }
            deleteLines(numbers);
            showCode();
            return;
        }

        if (cmd == ".clear") {
            headers.clear();
            code.clear();
            lastLineNumber = 0;
            return;
        }

        if (line.startsWith('#') || line.startsWith("using ")) {
            headers << line;
            lastLineNumber = headers.count();
        } else {
            if (!line.isEmpty()) {
                code << line;
                lastLineNumber = headers.count() + code.count();
            }
        }

        if (waitForReadyStdInputRead(40)) {
            // continue reading
            promptOut.off();
            return;
        }

        // compile
        compile();
    };

    print() << "cpi> " << flush;

    while (!end) {
        bool ready = waitForReadyStdInputRead(50);
        if (ready) {
            readCodeAndCompile();
        }
    }

    return 0;
}


int main(int argv, char *argc[])
{
    QCoreApplication app(argv, argc);
    app.setApplicationVersion(CPI_VERSION_STR);

    QCommandLineParser parser;
    parser.setApplicationDescription("Tiny C++ Interpreter.\nRuns in interactive mode by default.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "File to compile.","[file]");
    parser.addPositionalArgument("-", "Reads from stdin.","[-]");
    parser.process(app);

#if (defined Q_OS_WIN) || (defined Q_OS_DARWIN)
    conf = std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope, "cpi/cpi");
#else
    conf = std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope, "cpi/cpi");
#endif

    QFile confFile(conf->fileName());
    if (!confFile.exists()) {
        QFileInfo(confFile).absoluteDir().mkpath(".");

        if (confFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            confFile.write(DEFAULT_CONFIG);
            confFile.close();
        }
        conf->sync();
    }

#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(signalHandler, TRUE);
#else
    watchUnixSignal(SIGTERM);
    watchUnixSignal(SIGINT);
#endif

    int ret;
    QString file = isSetFileOption();
    Compiler compiler;

    if (!file.isEmpty()) {
        ret = compiler.compileFileAndExecute(file);
        if (ret) {
            compiler.printLastCompilationError();
        }
    } else if (QCoreApplication::arguments().contains("-")) {  // Check pipe option
        QString src;
        QTextStream tsstdin(stdin);
        tsstdin.setEncoding(QStringConverter::System);
        while (!tsstdin.atEnd()) {
            src += tsstdin.readAll();
        }

        ret = compiler.compileAndExecute(src);
        if (ret) {
            compiler.printLastCompilationError();
        }
    } else {
        // Check compiler
        Compiler::cxx();
        ret = interpreter();
    }
    return ret;
}
