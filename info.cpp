#include "info.h"

void InfoResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if (request.method() != "GET") {
     reply.httpReturn(403, "Only GET method is support by the remote control");
     return;
  }
  QueryHandler q("/info", request);
  StreamExtension se(&out);

  if (q.isFormat(".xml")) {
    reply.addHeader("Content-Type", "text/xml; charset=utf-8");
    replyXml(se);
  } else if (q.isFormat(".json")) {
    reply.addHeader("Content-Type", "application/json; charset=utf-8");
    replyJson(se);
  } else if (q.isFormat(".html")) { 
    reply.addHeader("Content-Type", "text/html; charset=utf-8");
    replyHtml(se);
  }
}

void InfoResponder::replyHtml(StreamExtension& se)
{
  if ( !se.writeBinary("/var/lib/vdr/plugins/restfulapi/API.html") ) {
     se.write("Copy API.html to /var/lib/vdr/plugins/restfulapi");
  }
}

void InfoResponder::replyJson(StreamExtension& se)
{
  time_t now = time(0);

  cxxtools::JsonSerializer serializer(*se.getBasicStream());
  serializer.serialize("0.0.1", "version");
  serializer.serialize((int)now, "time");
  
  std::vector< struct SerService > services;
  struct SerService s;
  s.Path = "/about"; s.Version = 1;
  services.push_back(s);
  s.Path = "/channels"; s.Version = 1;
  services.push_back(s);
  s.Path = "/events"; s.Version = 1;
  services.push_back(s);
  s.Path = "/recordings"; s.Version = 1;
  services.push_back(s);
  s.Path = "/remote"; s.Version = 1;
  services.push_back(s);
  s.Path = "/timers"; s.Version = 1;
  services.push_back(s);

  serializer.serialize(services, "services");
  serializer.finish();  
}

void InfoResponder::replyXml(StreamExtension& se)
{
  time_t now = time(0);

  se.writeXmlHeader();
  se.write("<info xmlns=\"http://www.domain.org/restfulapi/2011/info-xml\">\n");
  se.write(" <version>0.0.1</version>\n");
  se.write((const char*)cString::sprintf(" <time>%i</time>\n", (int)now)); 
  se.write(" <services>\n");
  se.write((const char*)cString::sprintf("  <service path=\"/about\" version=\"%i\" />\n", 1));
  se.write((const char*)cString::sprintf("  <service path=\"/channels\" version=\"%i\" />\n", 1));
  se.write((const char*)cString::sprintf("  <service path=\"/events\" version=\"%i\" />\n", 1));
  se.write((const char*)cString::sprintf("  <service path=\"/recordings\" version=\"%i\" />\n", 1));
  se.write((const char*)cString::sprintf("  <service path=\"/remote\" version=\"%i\" />\n", 1));
  se.write((const char*)cString::sprintf("  <service path=\"/timers\" version=\"%i\" />\n", 1));
  se.write(" </services>\n");
  se.write("</info>");
}

void operator<<= (cxxtools::SerializationInfo& si, const SerService& s)
{
  si.addMember("path") <<= s.Path;
  si.addMember("version") <<= s.Version;
}
