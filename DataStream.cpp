#include <httq/DataStream.h>
#include <httq/HttpRequest.h>

#include <QIODevice>
#include <QTcpSocket>
#include <QTimer>
#include <QDataStream>

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

  connect(mFrom, &QIODevice::readyRead,
          this, [this]()
  {
    DLOG << "DataStream read";

    mReadyRead = true;
    QTimer::singleShot(1, this, [this]() { readNext(); } );
  });

  connect(mTo, &QIODevice::bytesWritten,
          this, [this]()
  {
    DLOG << "DataStream write";

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
  DLOG << "readNext" << mPos;


  if (mPos > 0)
  {
    DLOG << "ignoring read until buffer was written";
    return;
  }


  if (!mReadyWrite)
  {
    DLOG << "won't read until write is ready!";
    return;
  }


  if (!mBodyPartial.isEmpty()) // don't read from device, but read from data that was already read when the connection was established
  {
    mBuffer.insert(0, mBodyPartial); // TODO: std::move

    mPos = mBodyPartial.size();
    DLOG << "mPos is now" << mPos;
    alreadyRead += mPos;
    DLOG << "prebody size" << mPos;

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

  mReadyRead = false;
  mPos = mFrom->read(mBuffer.data(), mBufferSize);
//  qWarning() << "mPos is now" << mPos;
//  qWarning() << "read" << mPos << "bytes";

  alreadyRead += mPos;

  if (mPos < 1)
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
  DLOG << "writeNext" << mPos;

  if (mPos < 1)
  {
    DLOG << "nothing to write";
    QTimer::singleShot(1, this, [this]() { readNext(); } );
    return;
  }

  if (/*!mTo->isOpen() || !mTo->isWritable() ||*/ !mReadyWrite /* wait until bytesWritten signal calls writeNext */)
  {
    WLOG << "!mReadyWrite";
    //readNext();
    return;
  }

  mReadyWrite = false;

  qint64 size = mTo->write(mBuffer.constData(), mPos);
  DLOG << "wrote to buffer" << mPos << "bytes"; //QString::fromUtf8(mBuffer.constData(), mPos);

  if (size < mPos)
  {
    WLOG << "cannot write all bytes";
    deleteLater();
    return;
  }

  mPos = 0;
  DLOG << "mPos is now" << mPos << "was set to 0 now!!";

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
    deleteLater();
    return;
  }



  // give the buffer a chance to emit a bytesWritten signal before trying to read again
  DLOG << "waiting for bytes written to be triggered...";
  QTimer::singleShot(1, this, [this]() { writeNext(); } );
}
