#ifndef __RECORDINGTRASH_H
#define __RECORDINGTRASH_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>

class RecordingTrashResponder : public cxxtools::http::Responder
{
public:
  explicit RecordingTrashResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
  }

  void reply(
    std::ostream& out,
    cxxtools::http::Request& request,
    cxxtools::http::Reply& reply) override;
};

typedef cxxtools::http::CachedService<RecordingTrashResponder>
  RecordingTrashService;

#endif
