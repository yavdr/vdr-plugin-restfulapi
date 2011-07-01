#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include "tools.h"
#include <time.h>
#include <vector>
#include <string>

class StatusResponder : public cxxtools::http::Responder
{
  public:
    explicit StatusResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service) { };
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void chunked(std::ostream& out, std::string text);
};

typedef cxxtools::http::CachedService<StatusResponder> StatusService;
