#ifndef PRINT_H
#define PRINT_H

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

#endif // PRINT_H
