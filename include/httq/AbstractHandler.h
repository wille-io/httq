#pragma once

#include <httq/AbstractServer.h>

#include <QVector>

#include <functional>


namespace httq
{
class AbstractHandler : public QObject
{
  Q_OBJECT
public:
#warning // TODO: parent!!
  AbstractHandler()
    : QObject(nullptr)
  {
//    QTimer *t = new QTimer();
//    connect(t, &QTimer::timeout,
//            this, [this]()
//    {
//      qWarning() << "I'm still alive!" << this->metaObject()->className();
//    });
//    t->setInterval(10000);
//    t->start();
  }

  HttpRequest *request() { return mRequest; }

protected:
  virtual void handle() = 0;
  void answer(int status); // empty json
  virtual void answer(int status, const QByteArray &content, const QString &contentType);
  void answer(int status, const QJsonObject &json);
  void answer(int status, const QJsonArray &json);
  void answer(int status, const QString &msg);

//private:
  friend class HandlerServer;

  void setRequest(HttpRequest *request) { mRequest = request; }
  HttpRequest *mRequest = nullptr;
};}
