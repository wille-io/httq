#pragma once

#include <httq/AbstractServer.h>
#include <httq/HandlerServer.h>

#include <QVector>

#include <functional>


class QWebSocket;


namespace httq
{
class AbstractWebSocketHandler : public QObject
{
  Q_OBJECT
public:
  AbstractWebSocketHandler(QWebSocket *ws);
//    : QObject(nullptr)
//  {
//     connect(ws, &QWebSocket::textMessage)

//  //    QTimer *t = new QTimer();
//  //    connect(t, &QTimer::timeout,
//  //            this, [this]()
//  //    {
//  //      qWarning() << "I'm still alive!" << this->metaObject()->className();
//  //    });
//  //    t->setInterval(10000);
//  //    t->start();
//  }
  
  QWebSocket *webSocket() { return mWs; }
  virtual void handleMessage(const QString &msg) = 0;
  
private slots:
  void _handleMessage(const QString &msg)
  {
    handleMessage(msg);
  }

protected:
//  void answer(int status); // empty json
//  virtual void answer(int status, const QByteArray &content, const QString &contentType);
//  void answer(int status, const QJsonObject &json);
//  void answer(int status, const QJsonArray &json);
//  void answer(int status, const QString &msg);
  Logger *logger() { return mLogger; }
  
private:
  friend class httq::HandlerServer;

  void setLogger(Logger *logger) { mLogger = logger; }
  void setRequest(HttpRequest *request) { mRequest = request; }
  //void setWs(QWebSocket *ws);
//  {
//    mWs = ws;
//    connect(ws, &QWebSocket)
//  }
  
  Logger *mLogger = nullptr;
  HttpRequest *mRequest = nullptr;
  QWebSocket *mWs = nullptr;
};


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

  HttpRequest &request() const { return *mRequest; }
  virtual void handle() = 0;

protected:
  void answer(int status); // empty json
  virtual void answer(int status, const QByteArray &content, const QString &contentType);
  void answer(int status, const QJsonObject &json);
  void answer(int status, const QJsonArray &json);
  void answer(int status, const QString &msg);
  Logger *logger() { return mLogger; }
  
private:
  friend class httq::HandlerServer;

  void setLogger(Logger *logger) { mLogger = logger; }
  void setRequest(HttpRequest *request) { mRequest = request; }
  
  Logger *mLogger = nullptr;
  HttpRequest *mRequest = nullptr;
};}
