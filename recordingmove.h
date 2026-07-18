#ifndef __RECORDINGMOVE_H
#define __RECORDINGMOVE_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>

class RecordingMoveResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingMoveResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingMoveResponder>
  RecordingMoveService;

#endif
