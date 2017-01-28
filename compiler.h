#include <QString>
#include <QByteArray>


class Compiler {
public:
    Compiler();
    ~Compiler();

    int compileAndExecute(const QString &src);
    int compileFileAndExecute(const QString &path);
    void printLastCompilationError() const;

    static bool isSetDebugOption();
    static bool isSetQtOption();

private:
    bool compile(const QByteArray &cmd, const QByteArray &code);

    QByteArray _sourceCode;
    QString _compileError;
};
