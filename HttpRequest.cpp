#include <httq/AbstractServer.h>
#include <httq/HttpRequest.h>
#include <httq/DataStream.h>

#include <http_parser.h>
#include <httq/Logger.h>

#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>


namespace httq
{
HttpRequest::HttpRequest(QTcpSocket *cli, AbstractServer *parent)
  : QObject(parent)
  , mCli(cli)
  , mLogger(parent->getLoggerFactory()->createLogger(this))
{
  mLogger->debug(QStringLiteral("HttpRequest c_tor"));

  mCli->setParent(this);

  // TODO: init on first read
  http_parser_init(&mParser, HTTP_REQUEST);
  mData.mHttpRquest = this;
  mParser.data = static_cast<void *>(&mData);

  connect(cli, &QTcpSocket::readyRead,
          this, &HttpRequest::slotReadyRead);
  connect(cli, &QTcpSocket::disconnected,
          this, &HttpRequest::deleteLater);



  // TODO: client timeout


  mParserSettings.on_message_begin = &HttpRequest::on_message_begin;
  mParserSettings.on_url = &HttpRequest::on_url;
  mParserSettings.on_status = &HttpRequest::on_status;
  mParserSettings.on_header_field = &HttpRequest::on_header_field;
  mParserSettings.on_header_value = &HttpRequest::on_header_value;
  mParserSettings.on_headers_complete = &HttpRequest::on_headers_complete;
  mParserSettings.on_body = &HttpRequest::on_body;
  mParserSettings.on_message_complete = &HttpRequest::on_message_complete;
  mParserSettings.on_chunk_header = &HttpRequest::on_chunk_header;
  mParserSettings.on_chunk_complete = &HttpRequest::on_chunk_complete;
}


HttpRequest::~HttpRequest()
{
  mLogger->debug(QStringLiteral("~HttpRequest"));

  if (mCli == nullptr)
    return;

  mLogger->debug(QStringLiteral("mCli flush & close"));
  mCli->flush();
  mCli->close();
}


QString HttpRequest::toString() const
{
  return QStringLiteral("HttpRequest(url = '%1', method = %2)").arg(url().toString()).arg(methodString());
}


int HttpRequest::on_message_begin(http_parser *parser)
{
  dataLogger(parser)->debug(QStringLiteral("on_message_begin"));
  return 0;
}


int HttpRequest::on_url(http_parser *parser, const char *at, size_t length)
{
  dataLogger(parser)->debug(QStringLiteral("on_url '%1'").arg(QString::fromUtf8(at, length)));

  // QString urlString(QString::fromUtf8(at, length));
  //qWarning() << "urlString" << urlString;

  http_parser_url parserUrl;
  if (http_parser_parse_url(at, length, false, &parserUrl) != 0)
  {
    dataLogger(parser)->debug(QStringLiteral("could not determine url! %1").arg(QString::fromUtf8(at, length)));
    return 1;
  }

  QUrl url;

#define isUrlFieldSet(field) parserUrl.field_set & (1 << field)
  if (isUrlFieldSet(UF_SCHEMA))
    url.setScheme(QString::fromUtf8(at + parserUrl.field_data[UF_SCHEMA].off, parserUrl.field_data[UF_SCHEMA].len));

  if (parserUrl.field_set & (1 << UF_HOST))
    url.setHost(QString::fromUtf8(at + parserUrl.field_data[UF_HOST].off, parserUrl.field_data[UF_HOST].len));

  if (parserUrl.field_set & (1 << UF_PORT))
  {
    bool k = false;
    QString portString(QString::fromUtf8(at + parserUrl.field_data[UF_PORT].off, parserUrl.field_data[UF_PORT].len));
    qint16 port = portString.toShort(&k);

    if (!k)
    {
      dataLogger(parser)->warning(QStringLiteral("could not determine port! %1").arg(portString));
      return 1;
    }

    url.setPort(static_cast<int>(port));
  }

  if (parserUrl.field_set & (1 << UF_PATH))
    url.setPath(QString::fromUtf8(at + parserUrl.field_data[UF_PATH].off, parserUrl.field_data[UF_PATH].len));

  HttpRequestData *reqData = data(parser);
  if (isUrlFieldSet(UF_QUERY))
  {
    // Qt 5 doesn't provide a method to return a QUrlQuery, but a QString version only, so generate it one-time and save it
    // TODO: I remember that there is indeed a way.
    QUrlQuery q(QString::fromUtf8(at + parserUrl.field_data[UF_QUERY].off, parserUrl.field_data[UF_QUERY].len));
    url.setQuery(q);
    reqData->mQuery = q;
  }

//  if (isUrlFieldSet(UF_FRAGMENT))
//    url.setFragment(QString::fromUtf8(at + parserUrl.field_data[UF_FRAGMENT].off, parserUrl.field_data[UF_FRAGMENT].len));

  if (isUrlFieldSet(UF_USERINFO))
    url.setUserInfo(QString::fromUtf8(at + parserUrl.field_data[UF_USERINFO].off, parserUrl.field_data[UF_USERINFO].len));
#undef isUrlFieldSet


  dataLogger(parser)->debug(QStringLiteral("final url = %1").arg(url.toString()));
  reqData->mUrl = url;


  return 0;
}


int HttpRequest::on_status(http_parser *parser, const char */*at*/, size_t /*length*/)
{
  dataLogger(parser)->debug(QStringLiteral("on_status"));
  return 0;
}


int HttpRequest::on_header_field(http_parser *parser, const char *at, size_t length)
{
  dataLogger(parser)->debug(QStringLiteral("on_header_field"));

  data(parser)->mCurrentHeaderField = QString::fromUtf8(at, length);

  return 0;
}


int HttpRequest::on_header_value(http_parser *parser, const char *at, size_t length)
{
  dataLogger(parser)->debug(QStringLiteral("on_header_value"));

  QString value(QString::fromUtf8(at, length));
  HttpRequestData *reqdata = data(parser);

  reqdata->mHeaders.insert(reqdata->mCurrentHeaderField, value);

  dataLogger(parser)->debug(QStringLiteral("added header: %1 = %2").arg(reqdata->mCurrentHeaderField).arg(value));

  reqdata->mCurrentHeaderField.clear(); // TODO: not really necessary

  return 0;
}


int HttpRequest::on_headers_complete(http_parser *parser)
{
  dataLogger(parser)->debug(QStringLiteral("on_headers_complete"));
  return 0;
}


int HttpRequest::on_body(http_parser *parser, const char *at, size_t length)
{
  dataLogger(parser)->debug(QStringLiteral("on_body"));
  data(parser)->mBodyPartial = std::move(QByteArray(at, length));

  //qWarning() << "BODY" << data(parser)->mBodyPartial;

  data(parser)->mDone = true; // as the rest of the body is read from out side, the parser is done at this point!

  return 0;
}


int HttpRequest::on_message_complete(http_parser *parser)
{
  dataLogger(parser)->debug(QStringLiteral("on_message_complete"));

  data(parser)->mDone = true;

  return 0;
}


int HttpRequest::on_chunk_header(http_parser *parser)
{
  dataLogger(parser)->debug(QStringLiteral("on_chunk_header"));
  return 0;
}


int HttpRequest::on_chunk_complete(http_parser *parser)
{
  dataLogger(parser)->debug(QStringLiteral("on_chunk_complete"));
  return 0;
}


qint64 HttpRequest::availableBytes() const
{
  return (mContentLength - alreadyRead);
}


DataStream *HttpRequest::createDataStreamToClient(int status, const QString &contentType, QIODevice *from, qint64 fileSize, qint64 bufferSize)
{
  mLogger->debug(QStringLiteral("createDataStreamToClient"));

  writeStatusHeader(status);
  addHeader(QStringLiteral("Content-Type"), contentType);
  addHeader(QStringLiteral("Content-Length"), QString::number(fileSize));
  writeHeaders();
  mCli->write("\r\n");

  return new DataStream(from, mCli, {}, fileSize, bufferSize, mLogger->getLoggerFactory(), this);
}


DataStream *HttpRequest::createDataStreamFromClient(QIODevice *to, qint64 fileSize, qint64 bufferSize)
{
  return new DataStream(mCli, to, mData.mBodyPartial, fileSize, bufferSize, mLogger->getLoggerFactory(), this);
}


//DataStream *HttpRequest::createDataStreamToFile(QIODevice *from, qint64 mBufferSize)
//{
//  return new DataStream(from, mCli, mBufferSize, mData.mBodyPartial, this);
//}


void HttpRequest::write(int status, const QJsonValue &jsonValue)
{
  QJsonDocument doc;

  if (jsonValue.isArray())
    doc = QJsonDocument(jsonValue.toArray());
  else
    doc = QJsonDocument(jsonValue.toObject());

  write(status, doc.toJson(), QStringLiteral("application/json"));
}


void HttpRequest::write(int status, const QByteArray &data, const QString &contentType)
{
  mLogger->debug(QStringLiteral("write to client: status = %1, data (size) = %2, contentType = %3").arg(status).arg(data.size()).arg(contentType));
  writeStatusHeader(status);

  addHeader(QStringLiteral("Content-Type"), contentType);
  addHeader(QStringLiteral("Content-Length"), QString::number(data.size()));

  writeHeaders();
  mCli->write("\r\n");

  mCli->write(data);
}


void HttpRequest::writeStatusHeader(int status)
{
  QString header(QStringLiteral("HTTP/1.1 %1 %2\r\n").arg(status).arg(QStringLiteral("OK") /* TODO: ! */));
  mLogger->debug(QStringLiteral("write to client: header = %1").arg(header));
  mCli->write(header.toUtf8());
}


void HttpRequest::addHeader(const QString &key, const QString &value)
{
  mHeaders.insert(key, value);
}


void HttpRequest::writeHeaders()
{
  QString headers;

  QMapIterator<QString, QString> it(mHeaders);
  while (it.hasNext())
  {
    it.next();
    headers += QStringLiteral("%1: %2\r\n").arg(it.key(), it.value());
  }

  mLogger->debug(QStringLiteral("write to client: headers = %1").arg(headers));
  mCli->write(headers.toUtf8());
}


void HttpRequest::slotReadyRead()
{
  // read until the whole header is done
  mLogger->debug(QStringLiteral("slotReadyRead"));

  if (mDone)
  {
    mLogger->debug(QStringLiteral("already marked as done"));
    return;
  }

  mCli->startTransaction(); // revertable, in the case that it's not a plain http request, but a ws request, which reads must be undone to be completely read again by the web socket server
  //qWarning() << "ready read"));

  while (mCli->bytesAvailable() && !mData.mDone)
  {
    mLogger->debug(QStringLiteral("bytes available"));
    const QByteArray data(mCli->read(10000));//HTTP_MAX_HEADER_SIZE - 1024));
    //const QByteArray data(mCli->readAll()); // TODO: TEMP!!!

    int dataSize = data.size();
    //qWarning() << "ready read - data" << dataSize << "avail" << mCli->bytesAvailable();
    size_t size = http_parser_execute(&mParser, &mParserSettings, data.constData(), static_cast<size_t>(dataSize));

    if (size < static_cast<size_t>(dataSize))
    {
      mLogger->error(QStringLiteral("http parser error: %1").arg(QString::fromUtf8(http_errno_name(HTTP_PARSER_ERRNO(&mParser)))));
      mCli->commitTransaction(); // not a ws connection at this point, it is legitimate to read the data
      deleteLater();
      return;
    }
  }


  if (mData.mDone)
  {
    mLogger->debug(QStringLiteral("now marked as done"));
    mDone = true;


    mLogger->debug(QStringLiteral("method = %1").arg(methodString()));


    // find Upgrade
    auto it = mData.mHeaders.constFind(QStringLiteral("Upgrade"));

    if (it != mData.mHeaders.constEnd() && (*it).toLower() == QStringLiteral("websocket")) // ws!
    {
      mLogger->debug(QStringLiteral("Upgrade: ").arg(*it));
      disconnect(mCli, &QTcpSocket::readyRead, nullptr, nullptr); // don't receive ready read events!

      mCli->rollbackTransaction();
      mCli->setParent(nullptr);
      auto cli = mCli;
      mCli = nullptr;
      emit signalUpgrade(cli);

      deleteLater();
      return;
    }


    // find Content-Length
    it = mData.mHeaders.constFind(QStringLiteral("Content-Length"));

    if (it != mData.mHeaders.constEnd())
    {
      bool k = false;
      mContentLength = (*it).toLongLong(&k);

      if (!k)
      {
        mLogger->error(QStringLiteral("invalid content length: ").arg(*it));
        deleteLater();
        return;
      }
    }


    alreadyRead = mData.mBodyPartial.size();

    mLogger->debug(QStringLiteral("already read %1 / %2 bytes").arg(alreadyRead).arg(contentLength()));

    mCli->commitTransaction(); // not a ws connection at this point, it is legitimate to read the data
    emit signalReady();
    return;
  }

  mLogger->debug(QStringLiteral("commit transaction"));
  mCli->commitTransaction(); // not a ws connection at this point, it is legitimate to read the data
}
}
