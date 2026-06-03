#include "changestate.h"

void ChangeStateResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
    QueryHandler::addHeader(reply);
    QueryHandler q("/change-state", request);

    if (request.method() == "OPTIONS") {
        reply.addHeader("Allow", "GET");
        reply.httpReturn(200, "OK");
        return;
    }

    if (request.method() != "GET") {
        reply.httpReturn(403, "Only GET method is supported");
        return;
    }

    StreamExtension se(&out);

    if (q.isFormat(".json")) {
        reply.addHeader("Content-Type", "application/json; charset=utf-8");
        replyJson(se);
    } else if (q.isFormat(".xml")) {
        reply.addHeader("Content-Type", "text/xml; charset=utf-8");
        replyXml(se);
    } else {
        reply.httpReturn(403, "Support formats: json and xml");
    }
}

void ChangeStateResponder::replyJson(StreamExtension& se)
{
    se.write("{\"statusVersion\":0,\"channelsVersion\":0,\"recordingsVersion\":0,\"timersVersion\":0,\"eventsVersion\":0}");
}

void ChangeStateResponder::replyXml(StreamExtension& se)
{
    se.writeXmlHeader();
    se.write("<change-state>");
    se.write("<statusVersion>0</statusVersion>");
    se.write("<channelsVersion>0</channelsVersion>");
    se.write("<recordingsVersion>0</recordingsVersion>");
    se.write("<timersVersion>0</timersVersion>");
    se.write("<eventsVersion>0</eventsVersion>");
    se.write("</change-state>");
}
