#include "changestate.h"
#include "changestatetracker.h"

#include <chrono>
#include <sstream>
#include <string>

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
        << "\"bootID\":\"" << StateChangeTracker::bootID << "\","
        << "\"channelsUpdate\":" << StateChangeTracker::lastChannelsUpdate() << ","
        << "\"recordingsUpdate\":" << StateChangeTracker::lastRecordingsUpdate() << ","
        << "\"timersUpdate\":" << StateChangeTracker::lastTimersUpdate() << ","
        << "\"eventsUpdate\":" << StateChangeTracker::lastEventsUpdate()
        << "}";

    std::string payload = json.str();
    se.write(payload.c_str());
}

void ChangeStateResponder::replyXml(StreamExtension& se)
{
    se.writeXmlHeader();
    se.write("<change-state>");
    se.write(cString::sprintf("<bootID>%s</bootID>", StateChangeTracker::bootID.c_str()));
    se.write(cString::sprintf("<channelsUpdate>%llu</channelsUpdate>", (unsigned long long)StateChangeTracker::lastChannelsUpdate()));
    se.write(cString::sprintf("<recordingsUpdate>%llu</recordingsUpdate>", (unsigned long long)StateChangeTracker::lastRecordingsUpdate()));
    se.write(cString::sprintf("<timersUpdate>%llu</timersUpdate>", (unsigned long long)StateChangeTracker::lastTimersUpdate()));
    se.write(cString::sprintf("<eventsUpdate>%llu</eventsUpdate>", (unsigned long long)StateChangeTracker::lastEventsUpdate()));
    se.write("</change-state>");
}
