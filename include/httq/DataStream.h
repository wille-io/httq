#pragma once

#include <httq/Logger.h>

#include <QObject>
#include <QScopedPointer>


class QIODevice;


namespace httq
{
class HttpRequest;

class DataStream : public QObject
{
  Q_OBJECT

private:
  explicit DataStream(QIODevice *from, QIODevice *to, const QByteArray &bodyPartial,
                      qint64 fileLength, size_t bufferSize, LoggerFactory *loggerFactory,
                      QObject *parent = nullptr);

public:
  ~DataStream();

private:
  friend class HttpRequest;

  void readNext();
  void writeNext();

  bool mReadyWrite { true };
  bool mReadyRead { false };
  QByteArray mBuffer;
  QByteArray mBodyPartial;
  QIODevice *mFrom;
  QIODevice *mTo;
  qint64 mBufferSize;
  qint64 alreadyRead { 0 };
  qint64 mFileLength;
  bool mFromSupportsSignals { false };
  bool mToSupportsSignals { false };
  QScopedPointer<Logger> mLogger;

signals:
  void signalError();
  void signalDone();

};
}
