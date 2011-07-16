#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include "tools.h"
#include <time.h>
#include <vector>
#include "statusmonitor.h"

struct SerService
{
  cxxtools::String Path;
  int Version;
  bool Internal;
};

struct SerPlugin
{
  cxxtools::String Name;
  cxxtools::String Version;
};

struct SerPluginList
{
  std::vector< struct SerPlugin > plugins;
};

struct SerPlayerInfo
{
  cxxtools::String Name;
  cxxtools::String FileName;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerService& s);
void operator<<= (cxxtools::SerializationInfo& si, const SerPlugin& p);
void operator<<= (cxxtools::SerializationInfo& si, const SerPluginList& pl);
void operator<<= (cxxtools::SerializationInfo& si, const SerPlayerInfo& pi);

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

