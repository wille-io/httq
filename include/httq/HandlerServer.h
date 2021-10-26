#pragma once

#include <httq/AbstractServer.h>

#include <QVector>

#include <functional>


class QWebSocket;


namespace httq
{
class AbstractHandler;

class HandlerDefinition
{
public:
  HandlerDefinition(const QString &method, const QString &path, const std::function<AbstractHandler *(void)> &handlerFactory)
    : mMethod(method)
    , mPath(path)
    , mHandlerFactory(handlerFactory)
  {}

  QString mMethod;
  QString mPath;
  std::function<AbstractHandler *(void)> mHandlerFactory;
};


class AbstractWebSocketHandler;


class WebSocketHandlerDefinition
{
public:
  WebSocketHandlerDefinition(const QString &path, const std::function<AbstractWebSocketHandler *(void)> &handlerFactory)
    : mPath(path)
    , mHandlerFactory(handlerFactory)
  {}

  QString mPath;
  std::function<AbstractWebSocketHandler *(void)> mHandlerFactory;
};


class HandlerServer : public httq::AbstractServer
{
  Q_OBJECT
public:
  explicit HandlerServer(QObject *parent = nullptr);
  virtual ~HandlerServer() = default;

  virtual bool newHttpConnection(HttpRequest *request) override;
  virtual QWebSocketServer *webSocketServer() const override { return mWsSvr; }

  virtual void missingHttpHandlerHandler(HttpRequest *request);
  virtual void missingWsHandlerHandler(QWebSocket *ws);

  bool addHandler(const QString &method, const QString &path, const std::function<AbstractHandler *(void)> &handlerFactory)
  {
    mHandlers += HandlerDefinition(method, path, handlerFactory);
    // TODO: don't add if already exists, etc.
    return true;
  }

  bool addWebSocketHandler(const QString &path, const std::function<AbstractWebSocketHandler *(void)> &handlerFactory)
  {
    mWebSocketHandlers += WebSocketHandlerDefinition(path, handlerFactory);
    // TODO: don't add if already exists, etc.
    return true;
  }

  bool handleWs(QWebSocket *ws, const QString &path, const QString &base);
  bool handle(HttpRequest *request, const QString &base);

protected:
  QString mBase;

private:
  QVector<HandlerDefinition> mHandlers; // TODO: matrix (method, path, etc.)
  QVector<WebSocketHandlerDefinition> mWebSocketHandlers; // TODO: matrix (method, path, etc.)
  QWebSocketServer *mWsSvr;
};
}
