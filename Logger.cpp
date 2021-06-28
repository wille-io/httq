#include <httq/Logger.h>

#include <QCoreApplication>
#include <QDebug>


namespace httq
{
// LoggerFactory
Logger *LoggerFactory::createLogger(QObject *parent)
{
  return new Logger(this, parent);
}


// Logger
Logger::Logger(LoggerFactory *loggerFactory, QObject *parent)
  : QObject(parent)
  , mLoggerFactory(loggerFactory)
{
  if (parent == nullptr)
    mPath = QStringLiteral("Global");
  else
  {
    mPath = QString::fromUtf8(parent->metaObject()->className());

//    do
//    {
//      mPath.prepend(QString::fromUtf8(parent->metaObject()->className()));
//      mPath.prepend(QStringLiteral(" => "));
//    }
//    while ((parent = parent->parent()));
//    mPath.remove(0, QString(QStringLiteral(" => ")).length()); // hacky - remove first separator

//    if (mPath.length() > 30)
//      mPath = QStringLiteral("...%1").arg(mPath.right(30));

    //qWarning() << "new Logger" << mPath;
  }
}


void Logger::debug(const QString &message)
{
  //qtLogger(QStringLiteral("DEBUG"), qDebug()) << message;
}


void Logger::warning(const QString &message)
{
  qtLogger(QStringLiteral("WARNING"), qWarning()) << message;
}


void Logger::error(const QString &message)
{
  qtLogger(QStringLiteral("ERROR"), qCritical()) << message;
}


QDebug Logger::qtLogger(const QString &type, QDebug logger)
{
  QDebug d(logger.noquote());
  d << type << mPath << ":";
  return d;
}
}
