#include <httq/AbstractBodyHandler.h>
#include <httq/BodyReader.h>
#include <httq/Logger.h>

#include <QTimer>

namespace httq
{
AbstractBodyHandler::AbstractBodyHandler(const std::optional<int> &bodySize, const std::optional<int> &timeoutMs)
  : AbstractHandler()
  , mBodySize(bodySize)
{
  if (!bodySize || !timeoutMs)
    return;

  QTimer *t = new QTimer(this);
  t->setInterval(timeoutMs.value());

  connect(t, &QTimer::timeout,
          this, [this]()
  {
    logger()->debug(QStringLiteral("timeout"));
    deleteLater();
  });

  t->start();
}


void AbstractBodyHandler::handle()
{
  if (!mBodySize)
  {
    bodyHandle();
    return;
  }

  BodyReader *br = new BodyReader(request()); // TODO: use body size

  connect(br, &BodyReader::signalDone,
          this, &AbstractBodyHandler::slotBodyHandle);
  connect(br, &BodyReader::signalError,
          this, &AbstractBodyHandler::slotBodyHandle);
  connect(br, &BodyReader::signalError,
          this, [this]()
  {
    logger()->debug(QStringLiteral("body reader failed"));
  });
}


void AbstractBodyHandler::slotBodyHandle()
{
  BodyReader *br = qobject_cast<BodyReader *>(sender()); // the sender is guaranteed to be of type BodyReader
  mBody = std::move(br->body());
  br->deleteLater();
  bodyHandle();
}


const QByteArray &AbstractBodyHandler::body() const
{
  return mBody;
}
}
