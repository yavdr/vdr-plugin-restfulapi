#include "femon.h"
using namespace std;

void FemonResponder::reply (std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler::addHeader(reply);
  QueryHandler q("/femon", request);

  if ( request.method() == "OPTIONS" ) {}

  if (request.method() != "GET") {
     reply.httpReturn(403, "Only GET method is supported by the femon service");
     return;
  }

  cPlugin *femon = cPluginManager::GetPlugin("femon");

  if (!femon) {
      reply.httpReturn(403, "Femon Plugin is not available. Please install first");
      return;
  }
  FemonService_v1_0 fe;

  femon->Service("FemonService-v1.0", &fe);

  StreamExtension se(&out);

  if (q.isFormat(".xml")) {
    reply.addHeader("Content-Type", "text/xml; charset=utf-8");
    replyXml(se, fe);
  } else if (q.isFormat(".json")) {
    reply.addHeader("Content-Type", "application/json; charset=utf-8");
    replyJson(se, fe);
  } else if (q.isFormat(".html")) {
    reply.addHeader("Content-Type", "text/html; charset=utf-8");
    replyHtml(se, fe);
  } else {
    reply.httpReturn(403, "Supported formats: xml, json and html!");
  }
};

void operator<<= (cxxtools::SerializationInfo& si, const FemonService_v1_0& fe)
{
  const char *name = fe.fe_name;
  const char *status = fe.fe_status;

  if (!name) {
      name = "None";
  }
  if (!status) {
      status = "Not available";
  }

  si.addMember("name") <<= name;
  si.addMember("status") <<= status;
  si.addMember("snr") <<= fe.fe_snr;
  si.addMember("signal") <<= fe.fe_signal;
  si.addMember("ber") <<= fe.fe_ber;
  si.addMember("unc") <<= fe.fe_unc;
  si.addMember("audio_bitrate") <<= fe.audio_bitrate;
  si.addMember("video_bitrate") <<= fe.video_bitrate;
  si.addMember("dolby_bitrate") <<= fe.dolby_bitrate;
}

void FemonResponder::replyHtml(StreamExtension se, FemonService_v1_0& fe) {};

void FemonResponder::replyJson(StreamExtension se, FemonService_v1_0& fe) {

  esyslog("Reply JSON");
  cxxtools::JsonSerializer serializer(*se.getBasicStream());
  serializer.serialize(fe, "femonData");
  serializer.finish();

};

void FemonResponder::replyXml(StreamExtension se, FemonService_v1_0& fe) {};
