#pragma once
#include <QTextStream>


class Print : public QTextStream {
public:
    static Print &globalInstance();
private:
    Print();
};


inline Print &print()
{
    return Print::globalInstance();
}
