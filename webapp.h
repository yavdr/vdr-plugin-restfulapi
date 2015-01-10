#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <locale.h>
#include <time.h>
#include "tools.h"

#ifndef WEBAPPESTFULAPI_H
#define WEBAPPRESTFULAPI_H

class WebappResponder : public cxxtools::http::Responder {
public:
  explicit WebappResponder(cxxtools::http::Service& service) : cxxtools::http::Responder(service) {}
  virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<WebappResponder> WebappService;

#endif // WEBAPPRESTFULAPI_H
