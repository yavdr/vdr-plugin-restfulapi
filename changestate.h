#ifndef __CHANGESTATE_H
#define __CHANGESTATE_H

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include "changestatetracker.h"
#include "tools.h"

class ChangeStateResponder : public cxxtools::http::Responder
{
public:
    explicit ChangeStateResponder(cxxtools::http::Service& service)
        : cxxtools::http::Responder(service) { };



    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyJson(StreamExtension& se);
    virtual void replyXml(StreamExtension& se);
};

typedef cxxtools::http::CachedService<ChangeStateResponder> ChangeStateService;

#endif // __CHANGESTATE_H
