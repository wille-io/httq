#include <httq/BodyReader.h>


BodyReader::BodyReader(httq::HttpRequest &request)
  : QObject()
  , mBuf(new QBuffer(&mBa))
{
  mBuf->open(QIODevice::WriteOnly);

  mDs = request.createDataStreamFromClient(mBuf, request.contentLength());

  connect(mDs, &QObject::destroyed,
    this, [this]()
    {
      emit signalDone();
    });

#if 0
  connect(mDs, &DataStream::signalDone,
          this, [this]()
  {
    LOG << "ds done";
  });

  connect(mDs, &DataStream::signalError,
  this, [this]()
  {
    LOG << "ds error";
  });
#endif

  connect(mDs, &httq::DataStream::signalError,
          this, &BodyReader::signalError);
}


BodyReader::~BodyReader()
{
  delete mBuf;
}


QByteArray &BodyReader::body()
{
  return mBa;
}
