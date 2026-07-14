#ifndef __RECORDINGMOVEPREVIEW_H
#define __RECORDINGMOVEPREVIEW_H

#include <cxxtools/http/reply.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/responder.h>

class RecordingMovePreviewResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingMovePreviewResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingMovePreviewResponder>
  RecordingMovePreviewService;

#endif
