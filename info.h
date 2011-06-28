#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include "tools.h"
#include <time.h>
#include <vector>

struct SerService
{
  cxxtools::String Path;
  int Version;
  bool Internal;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerService& s);

class InfoResponder : public cxxtools::http::Responder
{
  public:
    explicit InfoResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service) { };
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyHtml(StreamExtension& se);
    virtual void replyJson(StreamExtension& se);
    virtual void replyXml(StreamExtension& se);
};

typedef cxxtools::http::CachedService<InfoResponder> InfoService;

