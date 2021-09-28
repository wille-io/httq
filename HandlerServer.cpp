#include <httq/HandlerServer.h>
#include <httq/HttpRequest.h>
#include <httq/AbstractHandler.h>

#include <QJsonDocument>
#include <QJsonObject>



#include <httq/HttpRequest.h>



namespace httq
{
HandlerServer::HandlerServer(QObject *parent)
  : AbstractServer(parent)
{}


bool HandlerServer::newHttpConnection(HttpRequest *request)
{
//  qWarning() << "newHttpConnection";

//  //LOG << "headers" << request->requestHeaders().values().join(", ");
//  const QStringList cookies(request->requestHeaders().value("Cookie").split(";", Qt::SplitBehaviorFlags::SkipEmptyParts));
//  //LOG << "cookies" << cookies;

//  QString sessionString;
//  for (const QString &cookie : cookies) // auto findSession = std::find_if(cookies.constBegin(), cookies.constEnd(), [&session](const QString &cookie)
//  {
//    const QStringList cookiePair(cookie.simplified().split("=", Qt::SplitBehaviorFlags::SkipEmptyParts));

//    if (cookiePair.count() != 2 || cookiePair.at(0) != "session")
//      continue;

//    sessionString = cookiePair.at(1);
//    break;
//  }

  //qWarning() << "sessionString" << sessionString;
  if (!handle(request, mBase))
  {
    //qWarning() << "no handler found for" << request->toString();
    missingHandlerHandler(request);
    request->deleteLater();
    return false;
  }

  //LOG << "handler found for" << request->toString();

  return true;
}


void HandlerServer::missingHandlerHandler(HttpRequest *request)
{
  request->write(400, QJsonObject({{ "error", "no handler found" }}));
}


bool HandlerServer::handle(HttpRequest *request, const QString &base)
{
  auto it = std::find_if(mHandlers.begin(), mHandlers.end(), [request, &base](HandlerDefinition &handler)
  {
    const QString path(request->url().path().remove(base));

    QRegExp rx(handler.mPath);
    rx.setPatternSyntax(QRegExp::Wildcard);

    bool exem = rx.exactMatch(path);

    bool ret = ((handler.mMethod.toUpper() == request->methodString())
               && exem);

    //qWarning()/*LOG*/ << "handle: handler path:" << handler.mPath << "- request url:" << path << "- matches?" << ret << "- exem" << exem << "- request" << request->toString();

    return ret;
  });

  bool ret = (it != mHandlers.end());

  if (!ret)
    return false;

  AbstractHandler *handler = (*it).mHandlerFactory();

  connect(handler, &QObject::destroyed,
          request, &QObject::deleteLater); // delete request if handler was deleted (end-developer controlled environment)

  handler->setRequest(request);
  request->setParent(handler);

  /*return*/ handler->handle();
  return true;
}
}
