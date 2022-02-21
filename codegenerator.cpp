#include "codegenerator.h"
#include "compiler.h"
#include <QtCore/QtCore>

#define CPI_SRC                                                         \
    "#include <iostream>\n"                                             \
    "#include <string>\n"                                               \
    "#include <typeinfo>\n"                                             \
    "%1\n"                                                              \
    "%3\n"                                                              \
    "#define PRINT_IF(type)  if (ti == typeid(type)) { std::cout << (*(type *)p) << std::endl; }\n" \
    "\n"                                                                \
    "int main() {\n"                                                    \
    "%4\n"                                                              \
    "  %2\n"                                                            \
    "  void *p = (void *)&x_x;\n"                                       \
    "  const std::type_info &ti = typeid(x_x);\n"                       \
    "  if (ti == typeid(char *) || ti == typeid(unsigned char *) || ti == typeid(char const *)) {\n" \
    "    std::cout << '\\\"' << (*(char **)p) << '\\\"'<< std::endl;\n" \
    "  } else if (ti == typeid(std::string)) {\n"                       \
    "    std::cout << '\\\"' << (*(std::string *)p) << '\\\"'<< std::endl;\n" \
    "  } else if (ti == typeid(char)) {\n"                              \
    "    std::cout << (int)(*(char *)p) << std::endl;\n"                \
    "  } else if (ti == typeid(unsigned char)) {\n"                     \
    "    std::cout << (unsigned int)(*(unsigned char *)p) << std::endl;\n" \
    "  } else PRINT_IF(short)\n"                                        \
    "  else PRINT_IF(unsigned short)\n"                                 \
    "  else PRINT_IF(int)\n"                                            \
    "  else PRINT_IF(unsigned int)\n"                                   \
    "  else PRINT_IF(long)\n"                                           \
    "  else PRINT_IF(unsigned long)\n"                                  \
    "  else PRINT_IF(long long)\n"                                      \
    "  else PRINT_IF(unsigned long long)\n"                             \
    "  else PRINT_IF(float)\n"                                          \
    "  else PRINT_IF(double)\n"                                         \
    "  else if (ti == typeid(bool)) {\n"                                \
    "    if (*(bool *)p)\n"                                             \
    "      std::cout << \"true\" << std::endl;\n"                       \
    "    else\n"                                                        \
    "      std::cout << \"false\" << std::endl;\n"                      \
    "  }\n"                                                             \
    "%5\n"                                                              \
    "  else if (ti == typeid(void *)) {\n"                              \
    "    // print nothing\n"                                            \
    "  } else {\n"                                                      \
    "    // disable to print\n"                                         \
    "    std::cout << \"# disable to print : name:\" << ti.name() << \"  size:\" << sizeof(x_x) << std::endl;\n" \
    "  }\n"                                                             \
    "  return 0;\n"                                                     \
    "}"

#define QT_HEADERS                                                      \
    "#include <QtCore>\n"                                               \
    "#include <QStringList>\n"                                          \
    "#include <QChar>\n"                                                \
    "#include <QTextCodec>\n"

#define QT_INIT                                                         \
    "  QTextCodec *codec = QTextCodec::codecForName(\"UTF-8\");\n"      \
    "  QTextCodec::setCodecForLocale(codec);\n"                         \
    "#if QT_VERSION < 0x050000\n"                                       \
    "  QTextCodec::setCodecForTr(codec);\n"                             \
    "  QTextCodec::setCodecForCStrings(codec);\n"                       \
    "#endif\n"

#define QT_PARSE                                                        \
    "  else PRINT_IF(qint64)\n"                                         \
    "  else PRINT_IF(quint64)\n"                                        \
    "  else if (ti == typeid(QString)) {\n"                             \
    "    std::cout << '\\\"' << qPrintable(*(QString *)p) << '\\\"' << std::endl;\n" \
    "  } else if (ti == typeid(QLatin1String)) {\n"                     \
    "    std::cout << '\\\"' << ((QLatin1String *)p)->latin1() << '\\\"' << std::endl;\n" \
    "  } else if (ti == typeid(QChar)) {\n"                             \
    "    std::cout << '\\'' << qPrintable(QString(*(QChar *)p)) << '\\'' << std::endl;\n" \
    "  } else if (ti == typeid(QStringList) || ti == typeid(QList<QString>)) {\n" \
    "    QStringList list;\n"                                           \
    "    for (QListIterator<QString> i(*(QList<QString> *)p); i.hasNext(); )\n" \
    "      list << QChar('\\\"') + i.next() + '\\\"';\n"                \
    "    std::cout << '[' << qPrintable(list.join(\", \")) << ']' << std::endl;\n" \
    "  }\n"



CodeGenerator::CodeGenerator(const QString &headers, const QString &code)
   : _headers(headers), _code(code)
{ }


static QString modifyCode(const QString &code, bool safeCode = false)
{
#ifdef Q_CC_MSVC
    const QString func = "auto x_x = []{ %1}();";  // for MSVC
    if (code.isEmpty()) {
        return func.arg("");
    }

    QString mod;
    if (safeCode) {
        mod  = code;
        mod += "return (void*)0;";
    } else {
        const QRegularExpression re("[^;]+;\\s*$");
        auto match = re.match(code);
        if (match.hasMatch()) {
            mod = code;
            int pos = match.capturedStart();
            mod.insert(pos, QLatin1String("return "));
        }
    }
    return func.arg(mod);
#else
    QString func = safeCode ? "auto x_x = ({ %1 (void*)0;});\n" : "auto x_x = ({ %1});\n";  // for g++
    return func.arg(code);
#endif
}


QString CodeGenerator::generateMainFunc() const
{
    QString src;
    QString modified = modifyCode(_code, false);
    if (Compiler::isSetQtOption()) {
        src = QString(CPI_SRC).arg(_headers, modified, QT_HEADERS, QT_INIT, QT_PARSE);
    } else {
        src = QString(CPI_SRC).arg(_headers, modified, "", "", "");
    }
    return src;
}



QString CodeGenerator::generateMainFuncSafe() const
{
    QString src;
    QString modified = modifyCode(_code, true);
    if (Compiler::isSetQtOption()) {
        src = QString(CPI_SRC).arg(_headers, modified, QT_HEADERS, QT_INIT, QT_PARSE);
    } else {
        src = QString(CPI_SRC).arg(_headers, modified, "", "", "");
    }
    return src;
}
