#include <httq/AbstractHandler.h>
#include <httq/HttpRequest.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QString>
#include <QWebSocket>


namespace httq
{
/// AbstractWebSocketHandler
AbstractWebSocketHandler::AbstractWebSocketHandler(QWebSocket *ws)
  : QObject(nullptr) // TODO: parent ?
{
  mWs = ws;

  connect(ws, &QWebSocket::textMessageReceived,
          this, &AbstractWebSocketHandler::_handleMessage);
  connect(ws, &QWebSocket::disconnected,
          ws, &QWebSocket::deleteLater);
  connect(ws, &QWebSocket::destroyed,
          this, &AbstractWebSocketHandler::deleteLater);

//    QTimer *t = new QTimer();
//    connect(t, &QTimer::timeout,
//            this, [this]()
//    {
//      qWarning() << "I'm still alive!" << this->metaObject()->className();
//    });
//    t->setInterval(10000);
//    t->start();
}


QWebSocket *AbstractWebSocketHandler::webSocket()
{
  return mWs;
}


void AbstractWebSocketHandler::_handleMessage(const QString &msg)
{
  handleMessage(msg);
}


Logger *AbstractWebSocketHandler::logger()
{
  return mLogger;
}


void AbstractWebSocketHandler::setLogger(Logger *logger)
{
  mLogger = logger;
}


void AbstractWebSocketHandler::setRequest(HttpRequest *request)
{
  mRequest = request;
}



/// Abstract(Http)Handler
AbstractHandler::AbstractHandler()
  : QObject(nullptr)
{
#if 0
   QTimer *t = new QTimer();
   connect(t, &QTimer::timeout,
           this, [this]()
   {
     qWarning() << "I'm still alive!" << this->metaObject()->className();
   });
   t->setInterval(10000);
   t->start();
#endif
}


HttpRequest &AbstractHandler::request() const
{
  return *mRequest;
}


void AbstractHandler::answer(int status)
{
  answer(status, QJsonObject());
}


void AbstractHandler::answer(int status, const QJsonObject &json)
{
  assert(!mAlreadyAnswered);
  mAlreadyAnswered = true;
  connect(mRequest, &HttpRequest::signalWriteComplete,
          this, &QObject::deleteLater);
  mRequest->write(status, json);
}


void AbstractHandler::answer(int status, const QJsonArray &json)
{
  assert(!mAlreadyAnswered);
  mAlreadyAnswered = true;
  connect(mRequest, &HttpRequest::signalWriteComplete,
          this, &QObject::deleteLater);
  mRequest->write(status, json);
}


void AbstractHandler::answer(int status, const QByteArray &content, const QString &contentType) // main
{
  assert(!mAlreadyAnswered);
  mAlreadyAnswered = true;
  connect(mRequest, &HttpRequest::signalWriteComplete,
          this, &QObject::deleteLater);
  mRequest->write(status, content, contentType);
}


void AbstractHandler::answer(int status, const QString &msg)
{
  answer(status, msg.toUtf8(), "text/plain");
}


Logger *AbstractHandler::logger()
{
  return mLogger;
}


void AbstractHandler::setLogger(Logger *logger)
{
  mLogger = logger;
}


void AbstractHandler::setRequest(HttpRequest *request)
{
  mRequest = request;
}

}
