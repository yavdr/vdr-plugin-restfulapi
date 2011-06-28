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
  }else {
    reply.httpReturn(403, "Support formats: xml, json and html!");
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
  std::vector< RestfulService* > restful_services = RestfulServices::get()->Services(true, true);
  struct SerService s;
  for (size_t i = 0; i < restful_services.size(); i++) {
     s.Path = StringExtension::UTF8Decode(restful_services[i]->Path());
     s.Version = restful_services[i]->Version();
     s.Internal = restful_services[i]->Internal();
     services.push_back(s);
  }

  struct SerPluginList pl;

  cPlugin* p = NULL;
  int counter = 0;
  while ( (p = cPluginManager::GetPlugin(counter)) != NULL ) {
     struct SerPlugin sp;
     sp.Name = StringExtension::UTF8Decode(p->Name());
     sp.Version = StringExtension::UTF8Decode(p->Version());
     pl.plugins.push_back(sp);
     counter++;
  }

  serializer.serialize(services, "services");
  serializer.serialize(pl, "vdr");
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
  
  std::vector< RestfulService* > restful_services = RestfulServices::get()->Services(true, true);
  for (size_t i = 0; i < restful_services.size(); i++) {
    se.write((const char*)cString::sprintf("  <service path=\"%s\"  version=\"%i\" internal=\"%s\" />\n", 
              restful_services[i]->Path().c_str(),
              restful_services[i]->Version(),
              restful_services[i]->Internal() ? "true" : "false"));
  }
  se.write(" </services>\n");

  se.write(" <vdr>\n");
  se.write("  <plugins>\n");
 
  cPlugin* p = NULL; 
  int counter = 0;
  while ( (p = cPluginManager::GetPlugin(counter) ) != NULL ) {
     se.write((const char*)cString::sprintf("   <plugin name=\"%s\" version=\"%s\" />\n", p->Name(), p->Version()));
     counter++;
  }

  se.write("  </plugins>\n");
  se.write(" </vdr>\n");

  se.write("</info>");
}

void operator<<= (cxxtools::SerializationInfo& si, const SerService& s)
{
  si.addMember("path") <<= s.Path;
  si.addMember("version") <<= s.Version;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerPlugin& p)
{
  si.addMember("name") <<= p.Name;
  si.addMember("version") <<= p.Version;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerPluginList& pl)
{
  si.addMember("plugins") <<= pl.plugins;
}

