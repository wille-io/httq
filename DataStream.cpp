#include <httq/DataStream.h>
#include <httq/HttpRequest.h>

#include <QIODevice>
#include <QTcpSocket>
#include <QTimer>
#include <QDataStream>
#include <QTemporaryFile>
#include <QFile>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif


namespace httq
{
DataStream::DataStream(QIODevice *from, QIODevice *to, const QByteArray &bodyPartial,
                       qint64 fileLength, size_t bufferSize, LoggerFactory *loggerFactory,
                       QObject *parent)
  : QObject(parent)
  , mBodyPartial(bodyPartial)
  , mFrom(from)
  , mTo(to)
  , mBufferSize(bufferSize)
  , mFileLength(fileLength)
  , mLogger(loggerFactory->createLogger(this))
{
//#define SAVE_PARTIAL_BODY
#ifdef SAVE_PARTIAL_BODY
  {
    QTemporaryFile f;
    f.open();
    f.write(mBodyPartial);
    f.setAutoRemove(false);
    qWarning() << "partial:" << f.fileName();
  }
#endif


  if (mFileLength == 0) // basically, we are done here
  {
    mLogger->warning(QStringLiteral("file size = 0 - done"));
    //QMetaObject::invokeMethod() ...
    QTimer::singleShot(1, this, [this]() { emit signalDone(); deleteLater(); } ); // needs to be queued, so the developer has enough time to connect this signal to a slot first
    return;
  }

  //mLogger->debug(QStringLiteral("DataStream c_tor"));

  // TODO: timeout!

  mBuffer.reserve(bufferSize);


  // from QFile manpage: "Unlike other QIODevice implementations, such as QTcpSocket, QFile does not emit the aboutToClose(), bytesWritten(), or readyRead() signals."
  mFromSupportsSignals = (qobject_cast<QFileDevice *>(mFrom) == nullptr);
  mToSupportsSignals = (qobject_cast<QFileDevice *>(mTo) == nullptr);
  //mLogger->debug(QStringLiteral("signal support - from: %1 - to: %2").arg(mFromSupportsSignals).arg(mToSupportsSignals));


  if (mFromSupportsSignals)
  {
    connect(mFrom, &QIODevice::readyRead,
            this, [this]()
    {
      //mLogger->debug(QStringLiteral(">> DataStream read"));

      mReadyRead = true;
      QTimer::singleShot(1, this, [this]() { readNext(); } );
    });
  }


  if (mToSupportsSignals)
  {
    connect(mTo, &QIODevice::bytesWritten,
            this, [this]()
    {
      //mLogger->debug(QStringLiteral(">> DataStream write"));

      mReadyWrite = true;
      QTimer::singleShot(1, this, [this]() { readNext(); } );
    });
  }


  QTimer::singleShot(1, this, [this]() { readNext(); } );
}


DataStream::~DataStream()
{
  //mLogger->debug(QStringLiteral("~DataStream"));
}


void DataStream::readNext()
{
#if defined(DEBUG_SLOW) && defined(Q_OS_UNIX)
  //mLogger->debug(QStringLiteral("read: slowing down....."));
  usleep(2 * 1000 * 1000);
#endif

  //mLogger->debug(QStringLiteral("read: readNext: %1").arg(mBuffer.size()));


  if (mBuffer.size() > 0)
  {
    //mLogger->debug(QStringLiteral("read: ignoring read until buffer was written"));
    return;
  }


  if (mToSupportsSignals && !mReadyWrite)
  {
    //mLogger->debug(QStringLiteral("read: won't read until write is ready!"));
    return;
  }


  if (!mBodyPartial.isEmpty()) // don't read from device, but read from data that was already read when the connection was established
  {
    mBuffer = mBodyPartial;



//    qWarning() << "mBuffer content (partial):"));
//    for (int i = 0; (i < mBuffer.size() && i < 100); i++)
//      qWarning() << Qt::hex << mBuffer[i];



    alreadyRead += mBuffer.size();
    //mLogger->debug(QStringLiteral("read: prebody size: %1").arg(mBuffer.size()));

    mBodyPartial.clear();

    //mReadyWrite = true;
    //mLogger->debug(QStringLiteral("read: queueing writeNext after prebody read"));
    QTimer::singleShot(1, this, [this]() { /*qWarning() << "triggering writeNext and hoping that writeReady is true!!"));*/ writeNext(); } );
    return;
  }


  if (/*!mFrom->isOpen() || !mFrom->isReadable() || */ (!mFrom->bytesAvailable() && !mReadyRead))
  {
//    qWarning() << "nothing to read: bytes available" << mFrom->bytesAvailable() << "ready read" << mReadyRead;
//    qWarning() << "try reading anyways..." << mFrom->read(3000);
    //mLogger->debug(QStringLiteral("no bytes available and not ready read"));

    // XXX: TODO: FIXME: readyRead (sometimes) never arrives.... we need to have this extremely ugly workaround for now .... :'(
    QTimer::singleShot(1000, this, [this]() { readNext(); } ); // try again later...

    return;
  }

  // if (mFromSupportsSignals)
  //   mReadyRead = false;

  mBuffer = std::move(mFrom->read(mBufferSize));
  //mLogger->debug(QStringLiteral("read: mPos is now %1").arg(mBuffer.size()));


//  qWarning() << "read" << mPos << "bytes"));

  alreadyRead += mBuffer.size();

  if (mBuffer.size() < 1)
  {
    //mLogger->debug(QStringLiteral("read: cannot read anything"));
    deleteLater();
    return;
  }

  //mLogger->debug(QStringLiteral("read: queueing writeNext"));
  QTimer::singleShot(1, this, [this]() { writeNext(); } );
}


void DataStream::writeNext()
{
  //mLogger->debug(QStringLiteral("write: writeNext: %1").arg(mBuffer.size()));

  if (mBuffer.size() < 1)
  {
    //mLogger->debug(QStringLiteral("write: nothing to write"));
    QTimer::singleShot(1, this, [this]() { readNext(); } );
    return;
  }

  // TODO: check isOpen & isWritable maybe in c_tor?
  if (!mTo->isOpen() || !mTo->isWritable())
  {
    //mLogger->debug(QStringLiteral("write: file error: open? %1 - writable? %2").arg(mTo->isOpen()).arg(mTo->isWritable()));
    // TODO: signalDone??
    deleteLater();
    return;
  }

  if (mToSupportsSignals && !mReadyWrite /* wait until bytesWritten signal calls writeNext, but only if bytesWritten is supported, that is, when device is buffered */)
  {
    //mLogger->debug(QStringLiteral("write: !mReadyWrite"));
    //readNext();
    return;
  }

  if (mToSupportsSignals)
  {
    mReadyWrite = false;
  }

  const qint64 size = mTo->write(mBuffer);

  if (size < 0)
  {
    mLogger->warning(QStringLiteral("write: connection error while trying to write %1 bytes!").arg(mBuffer.size()));
    // TODO: signal error??
    deleteLater();
    return;
  }

  //mLogger->debug(QStringLiteral("write: wrote to buffer %1 bytes").arg(size)); //QString::fromUtf8(mBuffer.constData(), mPos);

  if (size < mBuffer.size())
  {
    mLogger->warning(QStringLiteral("write: cannot write all bytes: %1 / %2 bytes").arg(mBuffer.size()).arg(size));
    // TODO: signalDone??
    deleteLater();
    return;
  }

  mBuffer.clear();
  //mLogger->debug(QStringLiteral("write: mPos is now %1 and was set to 0 now!!").arg(mBuffer.size()));

  if (alreadyRead == mFileLength) // done
  {
    //mLogger->debug(QStringLiteral("write: DONE!"));
    emit signalDone();
    deleteLater();
    return;
  }

  if (alreadyRead > mFileLength)
  {
    //mLogger->debug(QStringLiteral("write: oops... we read too much!! data may be corrupt!! already read %1 bytes, but content length is only %2 (%1 / %2)").arg(alreadyRead).arg(mFileLength));
    emit signalDone(); // TODO: emit error??
    deleteLater();
    return;
  }


  // give the buffer a chance to emit a bytesWritten signal before trying to read again
  if (mToSupportsSignals)
  {
    //mLogger->debug(QStringLiteral("write: waiting for bytes written to be triggered..."));
    return; // wait for bytesWritten
  }

  //mLogger->debug(QStringLiteral("write: queueing writeNext again"));
  QTimer::singleShot(1, this, [this]() { writeNext(); } );
}
}
