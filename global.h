#pragma once
#include <QtCore>
#include <QString>
#include <memory>


namespace cpi {

#if QT_VERSION < 0x060000
const auto SkipEmptyParts = QString::SkipEmptyParts;

#else
const auto SkipEmptyParts = Qt::SkipEmptyParts;
const auto flush = Qt::flush;
const auto endl = Qt::endl;
#endif

}


extern std::unique_ptr<QSettings> conf;
extern QStringList cppsArgs;
extern QString aoutName();
