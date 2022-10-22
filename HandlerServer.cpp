#include <httq/HandlerServer.h>
#include <httq/HttpRequest.h>
#include <httq/AbstractHandler.h>
#include <httq/HttpRequest.h>
#include <httq/Logger.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocketServer>
#include <QWebSocket>


namespace httq
{
HandlerServer::HandlerServer(QObject *parent)
  : AbstractServer(parent)
  , mWsSvr(new QWebSocketServer("httq", QWebSocketServer::SslMode::NonSecureMode, this))
{
  connect(mWsSvr, &QWebSocketServer::newConnection,
         this, [this]()
  {
    QWebSocket *ws = mWsSvr->nextPendingConnection();
    //ws->setParent(this);
    
    connect(ws, &QWebSocket::disconnected,
            ws, &QObject::deleteLater);
    connect(ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            ws, &QObject::deleteLater);
    
    qWarning() << "ws client url" << ws->requestUrl().toString();
    
    QUrl url(ws->requestUrl());
    qWarning() << "path" << url.path();
    
    
    if (!handleWs(ws, url.path(), mBase))
    {
      //qWarning() << "no handler found for" << request->toString();
      missingWsHandlerHandler(ws);
      ws->deleteLater();
      return;
    }
  
    //LOG << "handler found for" << request->toString();
  });
}



bool HandlerServer::newHttpConnection(HttpRequest *request)
{
//  qWarning() << "newHttpConnection";

  if (!handle(request, mBase))
  {
    //qWarning() << "no handler found for" << request->toString();
    missingHttpHandlerHandler(request);
    request->deleteLater();
    return false;
  }

  //LOG << "handler found for" << request->toString();

  return true;
}


void HandlerServer::missingHttpHandlerHandler(HttpRequest *request)
{
  request->write(400, QJsonObject({{ "error", "no handler found" }}));
}


bool HandlerServer::newWebSocketConnection(QWebSocket *ws)
{
  qWarning() << "newWebSocketConnection";

  if (!handleWs(ws, ws->requestUrl().path(), mBase))
  {
    qWarning() << "no handler found for" << ws->requestUrl().path();
    missingWsHandlerHandler(ws);
    ws->deleteLater();
    return false;
  }

  qWarning() << "ws handler found for" << ws->requestUrl().path();

  return true;
}


void HandlerServer::missingWsHandlerHandler(QWebSocket *ws)
{
  ws->sendTextMessage("{ error: \"no websocket handler found\" }");
}


bool HandlerServer::handleWs(QWebSocket *ws, const QString &path, const QString &base)
{
  auto it = std::find_if(mWebSocketHandlers.begin(), mWebSocketHandlers.end(), [ws, &path](const WebSocketHandlerDefinition &handler)
  {
    bool pathMatch = QRegularExpression(QRegularExpression::wildcardToRegularExpression(handler.mPath)).match(path).hasMatch();
    bool ret = pathMatch;

    qWarning()/*LOG*/ << "handleWs: handler path:" << handler.mPath << "- request url:" << path << "- path match?" << pathMatch;

    return ret;
  });

  bool ret = (it != mWebSocketHandlers.end());

  if (!ret)
    return false;

  AbstractWebSocketHandler *handler = (*it).mHandlerFactory(ws);
  handler->setLogger(getLoggerFactory()->createLogger(handler));

#warning ?????
  // ws->setParent(handler); ?????

  connect(handler, &QObject::destroyed,
          ws, &QObject::deleteLater); // delete request if handler was deleted (end-developer controlled environment)


  return true;






/*  QUrl url(ws->requestUrl());
  QString _path(url.path().remove(base));

  connect(handler, &QObject::destroyed,
          ws, &QObject::deleteLater); // delete request if handler was deleted (end-developer controlled environment)

  
  connect(ws, &QWebSocket::textMessageReceived,
          handler, &AbstractWebSocketHandler::_handleMessage);
  
  return true;
*/
}


bool HandlerServer::handle(HttpRequest *request, const QString &base)
{
  const QString path(request->url().path().remove(base));
  auto it = std::find_if(mHandlers.begin(), mHandlers.end(), [request, &path](const HandlerDefinition &handler)
  {
    bool pathMatch = QRegularExpression(QRegularExpression::wildcardToRegularExpression(handler.mPath)).match(path).hasMatch();
    bool methodMatch = (handler.mMethod.toUpper() == request->methodString());

    bool ret = (methodMatch && pathMatch);

    //qWarning()/*LOG*/ << "handle: handler path:" << handler.mPath << "- request url:" << path << "- path match?" << pathMatch << "- method match?" << methodMatch << "- request" << request->toString();

    return ret;
  });

  bool ret = (it != mHandlers.end());

  if (!ret)
    return false;

  AbstractHandler *handler = (*it).mHandlerFactory();
  handler->setLogger(getLoggerFactory()->createLogger(handler));

  connect(handler, &QObject::destroyed,
          request, &QObject::deleteLater); // delete request if handler was deleted (end-developer controlled environment)

  handler->setRequest(request);
  request->setParent(handler);

  /*return*/ handler->handle();
  return true;
}
}
