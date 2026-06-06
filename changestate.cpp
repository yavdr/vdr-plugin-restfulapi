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
    se.write(cString::sprintf("<statusVersion>%llu</statusVersion>", (unsigned long long)ChangeStateCounter::StatusVersion()));
    se.write(cString::sprintf("<channelsVersion>%llu</channelsVersion>", (unsigned long long)ChangeStateCounter::ChannelsVersion()));
    se.write(cString::sprintf("<recordingsVersion>%llu</recordingsVersion>", (unsigned long long)ChangeStateCounter::RecordingsVersion()));
    se.write(cString::sprintf("<timersVersion>%llu</timersVersion>", (unsigned long long)ChangeStateCounter::TimersVersion()));
    se.write(cString::sprintf("<eventsVersion>%llu</eventsVersion>", (unsigned long long)ChangeStateCounter::EventsVersion()));
    se.write("</change-state>");
}
