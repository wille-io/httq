#include <httq/AbstractServer.h>
#include <httq/HttpRequest.h>

#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocketServer>


AbstractServer::AbstractServer(QObject *parent)
  : QObject(parent)
  , mSvr(new QTcpServer(this))
{
  connect(mSvr, &QTcpServer::newConnection,
          this, [this]()
  {
    while (QTcpSocket *cli = mSvr->nextPendingConnection())
    {
      if (cli == nullptr)
        return;

      HttpRequest *req = new HttpRequest(cli, this);

      connect(req, &HttpRequest::signalUpgrade,
              this, [this, req](QTcpSocket *sock)
      {
        QWebSocketServer *wsSvr = webSocketServer();

        if (wsSvr == nullptr)
        {
          sock->close();
          sock->deleteLater();

          qWarning() << "no ws svr to handle ws connection!";

          return;
        }

        wsSvr->handleConnection(sock);
        emit sock->readyRead(); // rollbackTransaction doesn't re-emit the readyRead signal
      });

      connect(req, &HttpRequest::signalReady,
              this, [this, req]()
      {
        newHttpConnection(req); // TODO: who owns HttpRequest now?
      });
    }
  });
}


void AbstractServer::slotReadyRead()
{

}


bool AbstractServer::listen(qint16 port, const QHostAddress &host)
{
  return mSvr->listen(host, port);
}
