#pragma once
#include <QByteArray>
#include <QString>


class Compiler {
public:
    Compiler();
    ~Compiler();

    int compileAndExecute(const QString &cc, const QStringList &options, const QString &src);
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
    bool compile(const QString &cc, const QStringList &options, const QString &code);

    QString _sourceCode;
    QString _compileError;
};
