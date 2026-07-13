#ifndef __RECORDINGPREVIEW_H
#define __RECORDINGPREVIEW_H

#include <cxxtools/http/reply.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/responder.h>

class RecordingTrashPreviewResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingTrashPreviewResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingTrashPreviewResponder>
  RecordingTrashPreviewService;

#endif
