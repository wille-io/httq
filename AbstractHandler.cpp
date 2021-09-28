#include <httq/AbstractHandler.h>
#include <httq/HttpRequest.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QString>


namespace httq
{
void AbstractHandler::answer(int status)
{
  answer(status, QJsonObject());
}


void AbstractHandler::answer(int status, const QJsonObject &json)
{
  mRequest->write(status, json);
  deleteLater();
}


void AbstractHandler::answer(int status, const QJsonArray &json)
{
  mRequest->write(status, json);
  deleteLater();
}


void AbstractHandler::answer(int status, const QByteArray &content, const QString &contentType) // main
{
  mRequest->write(status, content, contentType);
  deleteLater();
}


void AbstractHandler::answer(int status, const QString &msg)
{
  answer(status, msg.toUtf8(), "text/plain");
}

}
