#ifndef __RECORDINGRENAME_H
#define __RECORDINGRENAME_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>

class RecordingRenameResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingRenameResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingRenameResponder>
  RecordingRenameService;

#endif
