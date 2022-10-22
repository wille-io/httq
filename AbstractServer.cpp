#include <httq/AbstractServer.h>
#include <httq/HttpRequest.h>
#include <httq/Logger.h>

#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocketServer>


namespace httq
{
AbstractServer::AbstractServer(QObject *parent)
  : QObject(parent)
  , mSvr(new QTcpServer(this))
  , mWsSvr(new QWebSocketServer("", QWebSocketServer::SslMode::NonSecureMode, this)) // TODO: server name!
  , mLoggerFactory(nullptr)
  , mLogger(nullptr)//mLoggerFactory->createLogger(this))
{
  connect(mWsSvr, &QWebSocketServer::newConnection,
          this, [this]()
  {
    newWebSocketConnection(mWsSvr->nextPendingConnection());
  });
}


LoggerFactory *AbstractServer::createLoggerFactory()
{
  return new LoggerFactory(this);
}


LoggerFactory *AbstractServer::getLoggerFactory()
{
  if (mLoggerFactory == nullptr)
    mLoggerFactory = createLoggerFactory();
  return mLoggerFactory;
}


void AbstractServer::slotNewConnection()
{
  mLogger->debug(QStringLiteral("newConnection"));

  while (QTcpSocket *cli = mSvr->nextPendingConnection())
  {
    mLogger->debug(QStringLiteral("nextPendingConnection"));

    if (cli == nullptr)
    {
      mLogger->debug(QStringLiteral("invalid tcp client"));
      return;
    }

    HttpRequest *req = new HttpRequest(cli, this);

    connect(req, &HttpRequest::signalUpgrade,
            this, [this, req](QTcpSocket *sock)
    {
      mLogger->debug(QStringLiteral("signalUpgrade"));

      mWsSvr->handleConnection(sock);
      emit sock->readyRead(); // rollbackTransaction doesn't re-emit the readyRead signal
    });

    connect(req, &HttpRequest::signalReady,
            this, [this, req]()
    {
      mLogger->debug(QStringLiteral("signalReady"));
      newHttpConnection(req); // TODO: who owns HttpRequest now?sss
    });
  }
}


bool AbstractServer::listen(qint16 port, const QHostAddress &host)
{
  mLoggerFactory = createLoggerFactory();
  mLogger = mLoggerFactory->createLogger(this);

  mLogger->debug(QStringLiteral("listen"));

  connect(mSvr, &QTcpServer::newConnection,
          this, &AbstractServer::slotNewConnection);

  return mSvr->listen(host, port);
}
}
