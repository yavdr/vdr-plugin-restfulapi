#ifndef __RECORDINGMOVEVALIDATE_H
#define __RECORDINGMOVEVALIDATE_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>

class RecordingMoveValidateResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingMoveValidateResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingMoveValidateResponder>
  RecordingMoveValidateService;

#endif
