#pragma once

#include <httq/AbstractHandler.h>
#include <httq/BodyReader.h>

#include <QTimer>

#include <optional>


namespace httq
{

class AbstractBodyHandler : public AbstractHandler
{
public:
  AbstractBodyHandler(const std::optional<int> &bodySize = {} /* TODO: use limit! */, const std::optional<int> &timeoutMs = {})
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
  
  void handle() override
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
  
  virtual void bodyHandle() = 0;
  
private slots:
  void slotBodyHandle()
  {
    BodyReader *br = qobject_cast<BodyReader *>(sender()); // the sender is guaranteed to be of type BodyReader
    mBody = std::move(br->body());
    br->deleteLater();
    bodyHandle();
  }
  
protected:  
  const QByteArray &body() const { return mBody; }
  
private:
  std::optional<int> mBodySize;
  QByteArray mBody;
};

}
