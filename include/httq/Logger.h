#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QPointer>


namespace httq
{
class Logger;


class LoggerFactory : public QObject
{
  Q_OBJECT

public:
  LoggerFactory(QObject *parent)
    : QObject(parent)
  {}

  virtual Logger *createLogger(QObject *parent);
};


class Logger : public QObject
{
  Q_OBJECT
public:
  Logger(LoggerFactory *loggerFactory, QObject *parent);

  virtual void debug(const QString &message);
  virtual void warning(const QString &message);
  virtual void error(const QString &message);

  virtual LoggerFactory *getLoggerFactory() const
  {
    Q_ASSERT(mLoggerFactory);
    return mLoggerFactory;
  }

protected:
  QPointer<LoggerFactory> mLoggerFactory;

private:
  QDebug qtLogger(const QString &type, QDebug debug);

  QString mPath;
};
}
