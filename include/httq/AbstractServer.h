#pragma once

#include <QObject>
#include <QHostAddress>


class QTcpServer;
class QWebSocketServer;
class QWebSocket;


namespace httq
{
class HttpRequest;
class LoggerFactory;
class Logger;


class AbstractServer : public QObject
{
  Q_OBJECT
public:
  explicit AbstractServer(QObject *parent = nullptr);

  virtual bool newHttpConnection(HttpRequest *request) = 0;
  virtual bool newWebSocketConnection(QWebSocket *ws) = 0;
  virtual LoggerFactory *createLoggerFactory();
  LoggerFactory *getLoggerFactory();
  bool listen(qint16 port, const QHostAddress &host = QHostAddress::Any);

private slots:
  void slotNewConnection();

private:
  QTcpServer *mSvr;
  QWebSocketServer *mWsSvr;
  LoggerFactory *mLoggerFactory { nullptr };
  Logger *mLogger;
};
}
