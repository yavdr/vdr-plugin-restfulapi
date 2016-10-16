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
  } else if (q.isFormat(".xml")) {
	reply.addHeader("Content-Type", "text/xml; charset=utf-8");
	replyXml(se, fe);
  } else {
    reply.httpReturn(403U, "Supported formats: JSON");
  }
};

void FemonResponder::replyJson(StreamExtension se, FemonService_v1_0& fe) {

  cxxtools::JsonSerializer serializer(*se.getBasicStream());
  serializer.serialize(fe, "femonData");
  serializer.finish();

};

void FemonResponder::replyXml(StreamExtension se, FemonService_v1_0& fe) {

  const char *name = fe.fe_name;
  const char *status = fe.fe_status;

  if (!name) {
      name = "None";
  }
  if (!status) {
      status = "Not available";
  }

  se.writeXmlHeader();
  se.write("<femon xmlns=\"http://www.domain.org/restfulapi/2011/femon-xml\">\n");
  se.write(cString::sprintf(" <name>%s</name>\n", StringExtension::encodeToXml(name).c_str()));
  se.write(cString::sprintf(" <status>%s</status>\n", StringExtension::encodeToXml(status).c_str()));
  se.write(cString::sprintf(" <snr>%i</snr>\n", fe.fe_snr));
  se.write(cString::sprintf(" <signal>%i</signal>\n", fe.fe_signal));
  se.write(cString::sprintf(" <ber>%u</ber>\n", fe.fe_ber));
  se.write(cString::sprintf(" <unc>%u</unc>\n", fe.fe_unc));
  se.write(cString::sprintf(" <audio_bitrate>%.0f</audio_bitrate>\n", fe.audio_bitrate));
  se.write(cString::sprintf(" <video_bitrate>%.0f</video_bitrate>\n", fe.video_bitrate));
  se.write(cString::sprintf(" <dolby_bitrate>%.0f</dolby_bitrate>\n", fe.dolby_bitrate));
  se.write("</femon>\n");

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
