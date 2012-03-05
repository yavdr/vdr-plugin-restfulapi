#include "info.h"
using namespace std;

void InfoResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  QueryHandler q("/info", request);

  if (request.method() != "GET") {
     reply.httpReturn(403, "Only GET method is support by the remote control");
     return;
  }
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
  StatusMonitor* statm = StatusMonitor::get();

  cxxtools::JsonSerializer serializer(*se.getBasicStream());
  serializer.serialize("0.0.1", "version");
  serializer.serialize((int)now, "time");
  
  vector< struct SerService > services;
  vector< RestfulService* > restful_services = RestfulServices::get()->Services(true, true);
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

  if ( statm->getRecordingName().length() > 0 || statm->getRecordingFile().length() > 0 ) {
     SerPlayerInfo pi;
     pi.Name = StringExtension::UTF8Decode(statm->getRecordingName());
     pi.FileName = StringExtension::UTF8Decode(statm->getRecordingFile());
     serializer.serialize(pi, "video");
  } else {
     string channelid = "";
     cChannel* channel = Channels.GetByNumber(statm->getChannel());
     if (channel != NULL) { 
        channelid = (const char*)channel->GetChannelID().ToString();
        serializer.serialize(channelid, "channel");
        cEvent* event = VdrExtension::getCurrentEventOnChannel(channel);
                
        string eventTitle = "";
        int start_time = -1;
	int duration = -1;
        int eventId = -1;

        if ( event != NULL) {
           eventTitle = event->Title();
	   start_time = event->StartTime();
           duration = event->Duration(),
           eventId = (int)event->EventID();	   
        }

        serializer.serialize(eventId, "eventid");
        serializer.serialize(start_time, "start_time");
	serializer.serialize(duration, "duration");
        serializer.serialize(StringExtension::UTF8Decode(eventTitle), "title");
     }
  }

  serializer.serialize(pl, "vdr");
  serializer.finish();  
}

void InfoResponder::replyXml(StreamExtension& se)
{
  time_t now = time(0);
  StatusMonitor* statm = StatusMonitor::get();


  se.writeXmlHeader();
  se.write("<info xmlns=\"http://www.domain.org/restfulapi/2011/info-xml\">\n");
  se.write(" <version>0.0.1</version>\n");
  se.write(cString::sprintf(" <time>%i</time>\n", (int)now)); 
  se.write(" <services>\n");
  
  vector< RestfulService* > restful_services = RestfulServices::get()->Services(true, true);
  for (size_t i = 0; i < restful_services.size(); i++) {
    se.write(cString::sprintf("  <service path=\"%s\"  version=\"%i\" internal=\"%s\" />\n", 
              restful_services[i]->Path().c_str(),
              restful_services[i]->Version(),
              restful_services[i]->Internal() ? "true" : "false"));
  }
  se.write(" </services>\n");

  
  if ( statm->getRecordingName().length() > 0 || statm->getRecordingFile().length() > 0 ) {
     se.write(cString::sprintf(" <video name=\"%s\">%s</video>\n", StringExtension::encodeToXml(statm->getRecordingName()).c_str(), StringExtension::encodeToXml(statm->getRecordingFile()).c_str()));
  } else {
     cChannel* channel = Channels.GetByNumber(statm->getChannel());
     string channelid = "";
     cEvent* event = NULL;
     if (channel != NULL) { 
        channelid = (const char*)channel->GetChannelID().ToString();
        event = VdrExtension::getCurrentEventOnChannel(channel);  
     }

     se.write(cString::sprintf(" <channel>%s</channel>\n", channelid.c_str()));
     if ( event != NULL) {
        string eventTitle = "";
        if ( event->Title() != NULL ) { eventTitle = event->Title(); }

	se.write(cString::sprintf(" <eventid>%i</eventid>\n", event->EventID()));
        se.write(cString::sprintf(" <start_time>%i</start_time>\n", (int)event->StartTime()));
	se.write(cString::sprintf(" <duration>%i</duration>\n", (int)event->Duration()));
	se.write(cString::sprintf(" <title>%s</title>\n", StringExtension::encodeToXml(eventTitle).c_str()));
     }
  }

  se.write(" <vdr>\n");
  se.write("  <plugins>\n");
 
  cPlugin* p = NULL; 
  int counter = 0;
  while ( (p = cPluginManager::GetPlugin(counter) ) != NULL ) {
     se.write(cString::sprintf("   <plugin name=\"%s\" version=\"%s\" />\n", p->Name(), p->Version()));
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

void operator<<= (cxxtools::SerializationInfo& si, const SerPlayerInfo& pi)
{
  si.addMember("name") <<= pi.Name;
  si.addMember("filename") <<= pi.FileName;
}

