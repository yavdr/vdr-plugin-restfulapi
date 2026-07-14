#ifndef __RECORDINGVALIDATE_H
#define __RECORDINGVALIDATE_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>

class RecordingTrashValidateResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingTrashValidateResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingTrashValidateResponder>
  RecordingTrashValidateService;

#endif
