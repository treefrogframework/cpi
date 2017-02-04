#include <QString>
#include <QByteArray>


class Compiler {
public:
    Compiler();
    ~Compiler();

    int compileAndExecute(const QString &cc, const QString &ccOptions, const QString &src);
    int compileAndExecute(const QString &src);
    int compileFileAndExecute(const QString &path);
    void printLastCompilationError() const;
    void printContextCompilationError() const;

    static bool isSetDebugOption();
    static bool isSetQtOption();
    static QString cxx();
    static QString cxxflags();
    static QString ldflags();

private:
    bool compile(const QString &cmd, const QString &code);

    QString _sourceCode;
    QString _compileError;
};
