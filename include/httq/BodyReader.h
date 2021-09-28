#pragma once

#include <httq/HttpRequest.h>
#include <httq/DataStream.h>

#include <QObject>
#include <QBuffer>


class BodyReader : public QObject
{
  Q_OBJECT

public:
  BodyReader(httq::HttpRequest *request)
    : QObject()
    , mBuf(new QBuffer(&mBa))
  {
    mBuf->open(QIODevice::WriteOnly);

    mDs = request->createDataStreamFromClient(mBuf, request->contentLength());

    connect(mDs, &QObject::destroyed,
    this, [this]()
    {
      emit signalDone(mBa);
    });

    // connect(mDs, &DataStream::signalDone,
    //         this, [this]()
    // {
    //   LOG << "ds done";
    // });

    // connect(mDs, &DataStream::signalError,
    // this, [this]()
    // {
    //   LOG << "ds error";
    // });

    connect(mDs, &httq::DataStream::signalError,
            this, &BodyReader::signalError);
  }

  ~BodyReader()
  {
    //LOG << "def";
    //delete mDs;
    delete mBuf;
  }

  const QByteArray &body() { return mBa; }

signals:
  void signalDone(const QByteArray &body);
  void signalError();

private:
  QByteArray mBa;
  httq::DataStream *mDs = nullptr;
  QBuffer *mBuf = nullptr;
};
