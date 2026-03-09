#pragma once

#include <httq/HttpRequest.h>
#include <httq/DataStream.h>

#include <QObject>
#include <QBuffer>


class BodyReader : public QObject
{
  Q_OBJECT

public:
  BodyReader(httq::HttpRequest &request);
  ~BodyReader();

  QByteArray &body();

signals:
  void signalDone();
  void signalError();

private:
  QByteArray mBa;
  httq::DataStream *mDs = nullptr;
  QBuffer *mBuf = nullptr;
};
