#include "femon.h"
using namespace std;

void FemonResponder::reply (std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler::addHeader(reply);
  QueryHandler q("/femon", request);

  if ((femon = cPluginManager::GetPlugin("femon")) == NULL) {
      reply.httpReturn(403U, "Femon Plugin is not available. Please install first");
      return;
  }

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET");
      reply.httpReturn(200U, "OK");
      return;
  }

  if (request.method() != "GET") {
     reply.httpReturn(403U, "Only GET method is supported by the femon service");
     return;
  }

  FemonService_v1_0 fe;
  StreamExtension se(&out);
  femon->Service("FemonService-v1.0", &fe);

  if (q.isFormat(".json")) {
    reply.addHeader("Content-Type", "application/json; charset=utf-8");
    replyJson(se, fe);
  } else {
    reply.httpReturn(403U, "Supported formats: JSON");
  }
};

void FemonResponder::replyJson(StreamExtension se, FemonService_v1_0& fe) {

  esyslog("restfulapi: Reply JSON");
  cxxtools::JsonSerializer serializer(*se.getBasicStream());
  serializer.serialize(fe, "femonData");
  serializer.finish();

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
