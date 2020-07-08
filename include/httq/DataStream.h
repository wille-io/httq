#ifndef DataStreamFromClient_H
#define DataStreamFromClient_H

#include <QObject>


class QIODevice;
class HttpRequest;


class DataStream : public QObject
{
  Q_OBJECT

private:
  explicit DataStream(QIODevice *from, QIODevice *to, const QByteArray &bodyPartial, qint64 fileLength, size_t bufferSize, QObject *parent = nullptr);

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
  qint64 mPos { 0 };
  qint64 mBufferSize;
  qint64 alreadyRead { 0 };
  qint64 mFileLength;

signals:
  void signalDone();

};

#endif // DataStreamFromClient_H
