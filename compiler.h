#include <QString>
#include <QByteArray>


class Compiler {
public:
    Compiler();
    ~Compiler();

    int compileAndExecute(const QString &cc, const QString &ccOptions, const QString &src);
    int compileAndExecute(const QString &src);
    int compileAndExecute(const QString &cccmd, const QString &src);
    int compileFileAndExecute(const QString &path);
    void printLastCompilationError() const;
    void printContextCompilationError() const;

    static bool isSetDebugOption();
    static bool isSetQtOption();
    static QByteArray cc();
    static QByteArray cflags();
    static QByteArray ldflags();

private:
    bool compile(const QByteArray &cmd, const QByteArray &code);

    QByteArray _sourceCode;
    QString _compileError;
};
