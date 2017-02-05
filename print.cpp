#include "print.h"


Print &Print::globalInstance()
{
    static Print global;
    global.flush();
    return global;
}


Print::Print() : QTextStream(stdout, QIODevice::WriteOnly) {}
