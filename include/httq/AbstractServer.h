#ifndef ABSTRACTSERVER_H
#define ABSTRACTSERVER_H

#include <QObject>
#include <QHostAddress>


class HttpRequest;
class QTcpServer;
class QWebSocketServer;


class AbstractServer : public QObject
{
  Q_OBJECT
public:
  explicit AbstractServer(QObject *parent = nullptr);

  virtual bool newHttpConnection(HttpRequest *request) = 0;
  virtual QWebSocketServer *webSocketServer() const { return nullptr; }

  bool listen(qint16 port, const QHostAddress &host = QHostAddress::Any);

private:
  QTcpServer *mSvr;

private slots:
  void slotReadyRead();


signals:

};

#endif // ABSTRACTSERVER_H
