#include "print.h"


Print &Print::globalInstance()
{
    static Print global;
#if QT_VERSION >= 0x060000
    global.setEncoding(QStringConverter::System);
#endif
    global.flush();
    return global;
}


Print::Print() :
    QTextStream(stdout) { }
