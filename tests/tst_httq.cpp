#include <httq/AbstractServer.h>
#include <httq/DataStream.h>
#include <httq/HttpRequest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QtTest/QtTest>

Q_DECLARE_METATYPE(httq::HttpRequest*)

namespace {
class TestServer : public httq::AbstractServer
{
  Q_OBJECT
public:
  using httq::AbstractServer::AbstractServer;

  bool newHttpConnection(httq::HttpRequest *request) override
  {
    mLastRequest = request;
    emit requestReady(request);
    return true;
  }

  bool newWebSocketConnection(QWebSocket * /*ws*/) override
  {
    return false;
  }

  httq::HttpRequest *lastRequest() const { return mLastRequest; }

signals:
  void requestReady(httq::HttpRequest *request);

private:
  httq::HttpRequest *mLastRequest = nullptr;
};
} // namespace

class HttqTests : public QObject
{
  Q_OBJECT
private:
  static QHostAddress testHost()
  {
    // QHostAddress::LocalHost may resolve to IPv6 (::1) depending on the host setup.
    // Binding explicitly to IPv4 keeps the tests stable in environments without IPv6 loopback.
    return QHostAddress(QStringLiteral("127.0.0.1"));
  }

  static bool waitForCount(QSignalSpy &spy, int count, int timeoutMs = 1000)
  {
    QElapsedTimer timer;
    timer.start();

    while (spy.count() < count && timer.elapsed() < timeoutMs)
    {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
      QTest::qWait(10);
    }

    return spy.count() >= count;
  }

  static bool listenAndConnect(TestServer &server, QTcpSocket &client, QString &error)
  {
    auto *tcpServer = server.findChild<QTcpServer*>();
    if (tcpServer == nullptr)
    {
      error = QStringLiteral("QTcpServer child not found");
      return false;
    }

    if (!server.listen(0, testHost()))
    {
      error = tcpServer->errorString();
      return false;
    }

    client.connectToHost(testHost(), tcpServer->serverPort());
    if (!client.waitForConnected(1000))
    {
      error = client.errorString();
      return false;
    }

    return true;
  }

  static httq::HttpRequest *sendRequest(QTcpSocket &client, QSignalSpy &requestSpy,
                                        const QByteArray &request, QString &error)
  {
    if (client.write(request) != request.size())
    {
      error = QStringLiteral("Could not write full request to client socket");
      return nullptr;
    }

    if (!client.waitForBytesWritten(1000))
    {
      error = client.errorString();
      return nullptr;
    }

    if (!waitForCount(requestSpy, 1))
    {
      error = QStringLiteral("Server did not emit requestReady in time");
      return nullptr;
    }

    auto req = qvariant_cast<httq::HttpRequest*>(requestSpy.takeFirst().at(0));
    if (req == nullptr)
    {
      error = QStringLiteral("requestReady emitted without a valid HttpRequest");
      return nullptr;
    }

    return req;
  }

private slots:
  void initTestCase()
  {
    qRegisterMetaType<httq::HttpRequest*>();
  }

  void parsesGetRequest()
  {
    TestServer server;
    QTcpSocket client;
    QString error;
    if (!listenAndConnect(server, client, error))
      QSKIP(qPrintable(QStringLiteral("Loopback listen/connect is unavailable in this environment: %1").arg(error)));

    QSignalSpy requestSpy(&server, &TestServer::requestReady);

    const QByteArray request =
      "GET /path?x=1&y=two HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "User-Agent: HttqTests\r\n"
      "\r\n";

    auto req = sendRequest(client, requestSpy, request, error);
    QVERIFY2(req != nullptr, qPrintable(error));

    QCOMPARE(req->methodString(), QStringLiteral("GET"));
    QCOMPARE(req->url().path(), QStringLiteral("/path"));
    QCOMPARE(req->query().queryItemValue(QStringLiteral("x")), QStringLiteral("1"));
    QCOMPARE(req->query().queryItemValue(QStringLiteral("y")), QStringLiteral("two"));
    QCOMPARE(req->requestHeaders().value(QStringLiteral("Host")), QStringLiteral("localhost"));
    QCOMPARE(req->requestHeaders().value(QStringLiteral("User-Agent")), QStringLiteral("HttqTests"));
    QCOMPARE(req->contentLength(), 0);
  }

  void parsesHostWithPortAndBody()
  {
    TestServer server;
    QTcpSocket client;
    QString error;
    if (!listenAndConnect(server, client, error))
      QSKIP(qPrintable(QStringLiteral("Loopback listen/connect is unavailable in this environment: %1").arg(error)));

    QSignalSpy requestSpy(&server, &TestServer::requestReady);

    const QByteArray body = "hello";
    QByteArray request =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.com:1234\r\n"
      "Content-Length: ";
    request += QByteArray::number(body.size());
    request += "\r\n\r\n";
    request += body;

    auto req = sendRequest(client, requestSpy, request, error);
    QVERIFY2(req != nullptr, qPrintable(error));

    QCOMPARE(req->methodString(), QStringLiteral("POST"));
    QCOMPARE(req->url().path(), QStringLiteral("/submit"));
    QCOMPARE(req->url().host(), QStringLiteral("example.com"));
    QCOMPARE(req->url().port(), 1234);
    QCOMPARE(req->contentLength(), static_cast<qint64>(body.size()));
    QCOMPARE(req->availableBytes(), 0);
  }

  void writesStatusLineWithReasonPhrase()
  {
    TestServer server;
    QTcpSocket client;
    QString error;
    if (!listenAndConnect(server, client, error))
      QSKIP(qPrintable(QStringLiteral("Loopback listen/connect is unavailable in this environment: %1").arg(error)));

    QSignalSpy requestSpy(&server, &TestServer::requestReady);

    const QByteArray request =
      "GET / HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n";

    auto req = sendRequest(client, requestSpy, request, error);
    QVERIFY2(req != nullptr, qPrintable(error));

    req->write(404, QByteArray("no"), QStringLiteral("text/plain"));

    QTRY_VERIFY(client.bytesAvailable() > 0);
    const QByteArray response = client.readAll();
    QVERIFY(response.contains("HTTP/1.1 404 Not Found\r\n"));
    QVERIFY(response.contains("Content-Type: text/plain\r\n"));
    QVERIFY(response.contains("Content-Length: 2\r\n"));
  }

  void streamsHundredMegabyteFileResponse()
  {
    constexpr qint64 fileSize = 100 * 1024 * 1024;

    QTemporaryFile file;
    QVERIFY(file.open());

    const QByteArray chunk(1024 * 1024, '@');
    for (int i = 0; i < 100; ++i)
      QCOMPARE(file.write(chunk), static_cast<qint64>(chunk.size()));
    QCOMPARE(file.size(), fileSize);
    QVERIFY(file.seek(0));

    TestServer server;
    QTcpSocket client;
    QByteArray response;
    connect(&client, &QTcpSocket::readyRead, this, [&client, &response]()
    {
      response += client.readAll();
    });

    QString error;
    if (!listenAndConnect(server, client, error))
      QSKIP(qPrintable(QStringLiteral("Loopback listen/connect is unavailable in this environment: %1").arg(error)));

    QSignalSpy requestSpy(&server, &TestServer::requestReady);

    const QByteArray request =
      "GET /download HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n";

    auto req = sendRequest(client, requestSpy, request, error);
    QVERIFY2(req != nullptr, qPrintable(error));

    auto *stream = req->createDataStreamToClient(200, QStringLiteral("application/octet-stream"), &file, fileSize);
    QVERIFY(stream != nullptr);

    QSignalSpy doneSpy(stream, &httq::DataStream::signalDone);
    QTRY_COMPARE(doneSpy.count(), 1);

    QTRY_VERIFY(response.contains("\r\n\r\n"));

    const qsizetype bodyOffset = response.indexOf("\r\n\r\n") + 4;
    QVERIFY(bodyOffset >= 4);

    QTRY_COMPARE(static_cast<qint64>(response.size() - bodyOffset), fileSize);

    const QByteArray headers = response.left(bodyOffset);
    const QByteArray body = response.mid(bodyOffset);

    QVERIFY(headers.contains("HTTP/1.1 200 OK\r\n"));
    QVERIFY(headers.contains("Content-Type: application/octet-stream\r\n"));
    QVERIFY(headers.contains(QByteArray("Content-Length: ")
                             + QByteArray::number(fileSize)
                             + "\r\n"));
    QCOMPARE(body.size(), fileSize);
    QCOMPARE(body.left(16), QByteArray(16, '@'));
    QCOMPARE(body.right(16), QByteArray(16, '@'));
  }
};

QTEST_MAIN(HttqTests)

#include "tst_httq.moc"
