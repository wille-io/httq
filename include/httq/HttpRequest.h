#pragma once

#include <http_parser.h>

#include <QJsonDocument>
#include <QObject>
#include <QByteArray>
#include <QUrl>
#include <QMap>
#include <QString>
#include <QUrlQuery>


class QTcpSocket;
class QIODevice;


namespace httq
{
class AbstractServer;
class Logger;
class DataStream;
class HttpRequest;


struct HttpRequestData
{
  QByteArray mData;
  QUrl mUrl;
  QString mCurrentHeaderField;
  QMap<QString, QString> mHeaders;
  bool mDone { false };
  QByteArray mBodyPartial;
  QUrlQuery mQuery;
  HttpRequest *mHttpRquest;
};


class HttpRequest : public QObject
{
  Q_OBJECT

private:
  explicit HttpRequest(QTcpSocket *cli, AbstractServer *parent = nullptr);
public:
  ~HttpRequest();

  qint64 availableBytes() const;
  http_method method() const { return (enum http_method)mParser.method; }
  QString methodString() const { return QString::fromUtf8(http_method_str(method())); }
  const QUrl &url() const { return mData.mUrl; }
  const QMap<QString, QString> &requestHeaders() const { return mData.mHeaders; }
  const QUrlQuery &query() const { return mData.mQuery; }
  qint64 contentLength() const { return mContentLength; }
  Logger *logger() { return mLogger; }
  QString toString() const;

  // response
  void addHeader(const QString &key, const QString &value);
  void write(int status, const QByteArray &data, const QString &contentType);
  void write(int status, const QJsonValue &jsonValue, QJsonDocument::JsonFormat jsonFormat = QJsonDocument::JsonFormat::Compact);
  DataStream *createDataStreamFromClient(QIODevice *to, qint64 fileSize, qint64 bufferSize = 1024 * 1024);
  DataStream *createDataStreamToClient(int status, const QString &contentType, QIODevice *from, qint64 fileSize, qint64 bufferSize = 1024 * 1024);

  friend class DataStream;


private:
  friend class AbstractServer;

  // response
  void writeStatusHeader(int status);
  void writeHeaders();

  static int on_message_begin(http_parser *parser);
  static int on_url(http_parser *parser, const char *at, size_t length);
  static int on_status(http_parser *parser, const char *at, size_t length);
  static int on_header_field(http_parser *parser, const char *at, size_t length);
  static int on_header_value(http_parser *parser, const char *at, size_t length);
  static int on_headers_complete(http_parser *parser);
  static int on_body(http_parser *parser, const char *at, size_t length);
  static int on_message_complete(http_parser *parser);
  static int on_chunk_header(http_parser *parser);
  static int on_chunk_complete(http_parser *parser);

  static HttpRequestData *data(http_parser *parser) { return static_cast<HttpRequestData *>(parser->data); }
  static Logger *dataLogger(http_parser *parser) { return static_cast<HttpRequestData *>(parser->data)->mHttpRquest->logger(); }


  QTcpSocket *mCli;
  http_parser mParser;
  http_parser_settings mParserSettings;
  HttpRequestData mData;
  bool mDone { false };
  qint64 mContentLength { 0 };
  qint64 alreadyRead { 0 };
  QMap<QString, QString> mHeaders;
  Logger *mLogger;


private slots:
  void slotReadyRead();


signals:
  void signalUpgrade(QTcpSocket *sock);
  void signalReady();

};
}
