#ifndef __RECORDINGRENAMEPREVIEW_H
#define __RECORDINGRENAMEPREVIEW_H

#include <cxxtools/http/reply.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/responder.h>

class RecordingRenamePreviewResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingRenamePreviewResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingRenamePreviewResponder>
  RecordingRenamePreviewService;

#endif
