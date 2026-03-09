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
  AbstractBodyHandler(const std::optional<int> &bodySize = {} /* TODO: use limit! */, const std::optional<int> &timeoutMs = {});
  void handle() override;
  
  virtual void bodyHandle() = 0;
  
private slots:
  void slotBodyHandle();
  
protected:  
  const QByteArray &body() const;
  
private:
  std::optional<int> mBodySize;
  QByteArray mBody;
};

}
