#include "changestate.h"
#include "changestatecounter.h"

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
    
std::ostringstream json;
json
    << "{"
    << "\"statusVersion\":" << ChangeStateCounter::StatusVersion() << ","
    << "\"channelsVersion\":" << ChangeStateCounter::ChannelsVersion() << ","
    << "\"recordingsVersion\":" << ChangeStateCounter::RecordingsVersion() << ","
    << "\"timersVersion\":" << ChangeStateCounter::TimersVersion() << ","
    << "\"eventsVersion\":" << ChangeStateCounter::EventsVersion()
    << "}";

std::string payload = json.str();
se.write(payload.c_str());

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
