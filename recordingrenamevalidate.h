#ifndef __RECORDINGRENAMEVALIDATE_H
#define __RECORDINGRENAMEVALIDATE_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>

class RecordingRenameValidateResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingRenameValidateResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingRenameValidateResponder>
  RecordingRenameValidateService;

#endif
