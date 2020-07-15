#include <httq/DataStream.h>
#include <httq/HttpRequest.h>

#include <QIODevice>
#include <QTcpSocket>
#include <QTimer>
#include <QDataStream>
#include <QFileDevice>

#include <QDebug>

#define _LOG "httq [" << __FILE__ << ":" << __LINE__ << "]"
#define WLOG qWarning() << _LOG
#define DLOG WLOG // qDebug() << _LOG


DataStream::DataStream(QIODevice *from, QIODevice *to, const QByteArray &bodyPartial, qint64 fileLength, size_t bufferSize, QObject *parent)
  : QObject(parent)
  , mBodyPartial(bodyPartial)
  , mFrom(from)
  , mTo(to)
  , mBufferSize(bufferSize)
  , mFileLength(fileLength)
{
  if (mFileLength == 0) // basically, we are done here
  {
    WLOG << "file size = 0 - done";
    QTimer::singleShot(1, this, [this]() { emit signalDone(); deleteLater(); } ); // needs to be queued, so the developer has enough time to connect this signal to a slot first
    return;
  }

  DLOG << "DataStream c_tor";

  // TODO: timeout!
  mBuffer.reserve(bufferSize);


  // from QFile manpage: "Unlike other QIODevice implementations, such as QTcpSocket, QFile does not emit the aboutToClose(), bytesWritten(), or readyRead() signals."
  mFromSupportsSignals = (qobject_cast<QFileDevice *>(mFrom) == nullptr);
  mToSupportsSignals = (qobject_cast<QFileDevice *>(mTo) == nullptr);
  qWarning() << "signal support - from:" << mFromSupportsSignals << "- to:" << mToSupportsSignals;


  if (mFromSupportsSignals)
    connect(mFrom, &QIODevice::readyRead,
            this, [this]()
    {
      //qWarning() << "DataStreamFromClient read";

      mReadyRead = true;
      QTimer::singleShot(1, this, [this]() { readNext(); } );
    });


  if (mToSupportsSignals)
    connect(mTo, &QIODevice::bytesWritten,
            this, [this]()
    {
      //qWarning() << "DataStreamFromClient write";

      mReadyWrite = true;
      QTimer::singleShot(1, this, [this]() { readNext(); } );
    });


  QTimer::singleShot(1, this, [this]() { readNext(); } );
}


DataStream::~DataStream()
{
  DLOG << "~DataStream !!!";
}


void DataStream::readNext()
{
  DLOG << "readNext" << mBuffer.size();


  if (mBuffer.size() > 0)
  {
    DLOG << "ignoring read until buffer was written";
    return;
  }


  if (mToSupportsSignals && !mReadyWrite)
  {
    DLOG << "won't read until write is ready!";
    return;
  }


  if (!mBodyPartial.isEmpty()) // don't read from device, but read from data that was already read when the connection was established
  {
    mBuffer = std::move(mBodyPartial);
    alreadyRead += mBuffer.size();
    mBodyPartial.clear();

    //mReadyWrite = true;
    DLOG << "queueing writeNext after prebody read";
    QTimer::singleShot(1, this, [this]() { /*qWarning() << "triggering writeNext and hoping that writeReady is true!!";*/ writeNext(); } );
    return;
  }


  if (/*!mFrom->isOpen() || !mFrom->isReadable() || */ (!mFrom->bytesAvailable() && !mReadyRead))
  {
//    qWarning() << "nothing to read: bytes available" << mFrom->bytesAvailable() << "ready read" << mReadyRead;
//    qWarning() << "try reading anyways..." << mFrom->read(3000);
    return;
  }

  if (mFromSupportsSignals)
    mReadyRead = false;

  mBuffer = mFrom->read(mBufferSize);
//  qWarning() << "mPos is now" << mPos;
//  qWarning() << "read" << mPos << "bytes";

  alreadyRead += mBuffer.size();

  if (mBuffer.size() < 1)
  {
    DLOG << "cannot read anything";
    deleteLater();
    return;
  }

  DLOG << "queueing writeNext";
  QTimer::singleShot(1, this, [this]() { writeNext(); } );
}


void DataStream::writeNext()
{
  DLOG << "writeNext" << mBuffer.size();

  if (mBuffer.size() < 1)
  {
    DLOG << "nothing to write";
    QTimer::singleShot(1, this, [this]() { readNext(); } );
    return;
  }

  // TODO: check isOpen & isWritable maybe in c_tor?
  if (!mTo->isOpen() || !mTo->isWritable())
  {
    WLOG << "file error: open?" << mTo->isOpen() << "- writable?" << mTo->isWritable();
    emit signalDone();
    deleteLater();
    return;
  }

  if (mToSupportsSignals && !mReadyWrite /* wait until bytesWritten signal calls writeNext */)
  {
    WLOG << "!mReadyWrite";
    //readNext();
    return;
  }

  if (mToSupportsSignals)
    mReadyWrite = false;

  qint64 size = mTo->write(mBuffer);
  DLOG << "wrote to buffer" << mBuffer.size() << "bytes"; //QString::fromUtf8(mBuffer.constData(), mPos);

  if (size < mBuffer.size())
  {
    WLOG << "cannot write all bytes";
    emit signalDone();
    deleteLater();
    return;
  }

  mBuffer.clear();
  DLOG << "mPos is now" << mBuffer.size() << "was set to 0 now!!";

  if (alreadyRead == mFileLength) // done
  {
    DLOG << "DONE!";
    emit signalDone();
    deleteLater();
    return;
  }

  if (alreadyRead > mFileLength)
  {
    DLOG << "oops... we read too much!! data may be corrupt!! already read" << alreadyRead << "bytes, but content length is only" << mFileLength << "("<<alreadyRead<<"/"<<mFileLength<<")";
    emit signalDone();
    deleteLater();
    return;
  }



  // give the buffer a chance to emit a bytesWritten signal before trying to read again
  if (mToSupportsSignals)
    DLOG << "waiting for bytes written to be triggered...";

  QTimer::singleShot(1, this, [this]() { writeNext(); } );
}
