#pragma once
#include <QString>
#include <QByteArray>

class CodeGenerator {
public:
    CodeGenerator(const QString &headers, const QString &code);
    QString generateMainFunc() const;
    QString generateMainFuncSafe() const;

private:
    QString _headers;
    QString _code;
};
