#include "print.h"


Print &Print::globalInstance()
{
    static Print global;
    global.setEncoding(QStringConverter::System);
    global.flush();
    return global;
}


Print::Print() : QTextStream(stdout) {}
