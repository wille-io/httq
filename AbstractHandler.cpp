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



/// Abstract(Http)Handler
void AbstractHandler::answer(int status)
{
  answer(status, QJsonObject());
}


void AbstractHandler::answer(int status, const QJsonObject &json)
{
  mRequest->write(status, json);
  deleteLater();
}


void AbstractHandler::answer(int status, const QJsonArray &json)
{
  mRequest->write(status, json);
  deleteLater();
}


void AbstractHandler::answer(int status, const QByteArray &content, const QString &contentType) // main
{
  mRequest->write(status, content, contentType);
  deleteLater();
}


void AbstractHandler::answer(int status, const QString &msg)
{
  answer(status, msg.toUtf8(), "text/plain");
}

}
