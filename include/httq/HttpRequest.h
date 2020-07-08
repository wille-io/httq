#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <http_parser.h>

#include <QObject>
#include <QByteArray>
#include <QUrl>
#include <QMap>
#include <QString>
#include <QUrlQuery>


class QTcpSocket;
class QIODevice;
class DataStream;


struct HttpRequestData
{
  QByteArray mData;
  QUrl mUrl;
  QString mCurrentHeaderField;
  QMap<QString, QString> mHeaders;
  bool mDone { false };
  QByteArray mBodyPartial;
  QUrlQuery mQuery;
};


class HttpRequest : public QObject
{
  Q_OBJECT

private:
  explicit HttpRequest(QTcpSocket *cli, QObject *parent = nullptr);
public:
  ~HttpRequest();

  qint64 availableBytes() const;
  http_method method() const { return (enum http_method)mParser.method; }
  const QUrl &url() const { return mData.mUrl; }
  const QUrlQuery &query() const { return mData.mQuery; }
  qint64 contentLength() const { return mContentLength; }
  //QTcpSocket *socket() { return mCli; }
  //QByteArray read(qint64 max);

  // response
  void addHeader(const QString &key, const QString &value);
  void write(int status, const QByteArray &data, const QString &contentType);
  void write(int status, const QJsonValue &jsonValue);
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


  QTcpSocket *mCli;
  http_parser mParser;
  http_parser_settings mParserSettings;
  HttpRequestData mData;
  bool mDone { false };
  qint64 mContentLength { 0 };
  qint64 alreadyRead { 0 };
  QMap<QString, QString> mHeaders;


private slots:
  void slotReadyRead();


signals:
  void signalUpgrade(QTcpSocket *sock);
  void signalReady();

};

#endif // HTTPREQUEST_H
