#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <vdr/tools.h>
#include "femon/femonservice.h"
#include "tools.h"


#ifndef FEMONRESTFULAPI_H
#define FEMONRESTFULAPI_H

class FemonResponder : public cxxtools::http::Responder {
private:
  cPlugin *femon;
public:
  explicit FemonResponder(cxxtools::http::Service& service)
        : cxxtools::http::Responder(service) { };
  virtual ~FemonResponder() {};
  virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
  virtual void replyJson(StreamExtension se, FemonService_v1_0& fe);
  virtual void replyXml(StreamExtension se, FemonService_v1_0& fe);
};

typedef cxxtools::http::CachedService<FemonResponder> FemonService;

void operator<<= (cxxtools::SerializationInfo& si, const FemonService_v1_0& fe);

#endif
