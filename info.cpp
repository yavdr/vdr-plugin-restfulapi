#include "info.h"
#include <vdr/videodir.h>
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
  } else {
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
  serializer.serialize("0.0.2", "version");
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
  pl.Version = StringExtension::UTF8Decode(VDRVERSION);

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

     int iCurrent = 0, iTotal = 0, iSpeed = -1;
     bool bPlay, bForward;
     string sPlay = " ";
     if (cControl *Control = cControl::Control(true)) {
        Control->GetIndex(iCurrent, iTotal);
        // Returns the current and total frame index
        Control->GetReplayMode(bPlay, bForward, iSpeed);
        // Returns the current replay mode (if applicable).
        // 'Play' tells whether we are playing or pausing, 'Forward' tells whether
        // we are going forward or backward and 'Speed' is -1 if this is normal
        // play/pause mode, 0 if it is single speed fast/slow forward/back mode
        // and >0 if this is multi speed mode.
        if (iSpeed == -1) {
           if (bPlay)
              sPlay = "playing";
           else
              sPlay = "pausing";
        }
        else if (iSpeed == 0) {
           if (bForward)
              sPlay = "forward";
           else
              sPlay = "backward";
        }
        else if (iSpeed > 0) {
           if (bForward)
              sPlay = "forward x "+StringExtension::itostr(iSpeed);
           else
              sPlay = "backward x "+StringExtension::itostr(iSpeed);
        }
     }
     serializer.serialize(StringExtension::UTF8Decode(sPlay), "replay_mode");
     serializer.serialize(iCurrent, "current_frame");
     serializer.serialize(iTotal, "total_frames");

     cRecording *recording = Recordings.GetByName(statm->getRecordingFile().c_str());
#if APIVERSNUM >= 10703
     if (recording) {
        serializer.serialize(recording->IsPesRecording(), "is_pes_recording");
        serializer.serialize(recording->FramesPerSecond(), "frames_per_second");
     } else {
        serializer.serialize(false, "is_pes_recording");
        serializer.serialize(DEFAULTFRAMESPERSECOND, "frames_per_second");
     }
#else
     if (recording) {
        serializer.serialize(true, "is_pes_recording");
     } else {
        serializer.serialize(false, "is_pes_recording");
     }
     serializer.serialize(FRAMESPERSEC, "frames_per_second");
#endif
  } else {
     string channelid = "";
     string channelname = "";
     int channelnumber = statm->getChannel();
     cChannel* channel = Channels.GetByNumber(channelnumber);
     if (channel != NULL) { 
        channelid = (const char*)channel->GetChannelID().ToString();
        serializer.serialize(channelid, "channel");
        channelname = channel->Name();
        serializer.serialize(channelname, "channel_name");
        serializer.serialize(channelnumber, "channel_number");
        cEvent* event = VdrExtension::getCurrentEventOnChannel(channel);

        string eventTitle = "";
        int start_time = -1;
        int duration = -1;
        int eventId = -1;

        if (event != NULL) {
           eventTitle = event->Title();
           start_time = event->StartTime();
           duration = event->Duration();
           eventId = (int)event->EventID();
        }

        serializer.serialize(eventId, "eventid");
        serializer.serialize(start_time, "start_time");
        serializer.serialize(duration, "duration");
        serializer.serialize(StringExtension::UTF8Decode(eventTitle), "title");
     }
  }

  serializer.serialize(statm->isRecord(), "recording");

  SerDiskSpaceInfo ds;
  ds.Description = cVideoDiskUsage::String(); //description call goes first, it calls HasChanged
  ds.UsedPercent = cVideoDiskUsage::UsedPercent();
  ds.FreeMB      = cVideoDiskUsage::FreeMB();
  ds.FreeMinutes = cVideoDiskUsage::FreeMinutes();
  serializer.serialize(ds, "diskusage");

  serializer.serialize(pl, "vdr");
  serializer.finish();  
}

void InfoResponder::replyXml(StreamExtension& se)
{
  time_t now = time(0);
  StatusMonitor* statm = StatusMonitor::get();

  se.writeXmlHeader();
  se.write("<info xmlns=\"http://www.domain.org/restfulapi/2011/info-xml\">\n");
  se.write(" <version>0.0.2</version>\n");
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
     int iCurrent = 0, iTotal = 0, iSpeed = -1;
     bool bPlay, bForward;
     string sPlay = " ";
     if (cControl *Control = cControl::Control(true)) {
        Control->GetIndex(iCurrent, iTotal);
        // Returns the current and total frame index
        Control->GetReplayMode(bPlay, bForward, iSpeed);
        // Returns the current replay mode (if applicable).
        // 'Play' tells whether we are playing or pausing, 'Forward' tells whether
        // we are going forward or backward and 'Speed' is -1 if this is normal
        // play/pause mode, 0 if it is single speed fast/slow forward/back mode
        // and >0 if this is multi speed mode.
        if (iSpeed == -1) {
           if (bPlay)
              sPlay = "playing";
           else
              sPlay = "pausing";
        }
        else if (iSpeed == 0) {
           if (bForward)
              sPlay = "forward";
           else
              sPlay = "backward";
        }
        else if (iSpeed > 0) {
           if (bForward)
              sPlay = "forward x "+StringExtension::itostr(iSpeed);
           else
              sPlay = "backward x "+StringExtension::itostr(iSpeed);
        }
     }
     se.write(cString::sprintf(" <replay_mode>%s</replay_mode>\n", sPlay.c_str()));
     se.write(cString::sprintf(" <current_frame>%d</current_frame>\n", iCurrent));
     se.write(cString::sprintf(" <total_frames>%d</total_frames>\n", iTotal));

     cRecording *recording = Recordings.GetByName(statm->getRecordingFile().c_str());
#if APIVERSNUM >= 10703
     if (recording) {
        se.write(cString::sprintf(" <is_pes_recording>%s</is_pes_recording>\n", recording->IsPesRecording() ? "true" : "false" ));
        se.write(cString::sprintf(" <frames_per_second>%0.0f</frames_per_second>\n", recording->FramesPerSecond()));
     } else {
        se.write(cString::sprintf(" <is_pes_recording>false</is_pes_recording>\n"));
        se.write(cString::sprintf(" <frames_per_second>%0.0f</frames_per_second>\n", DEFAULTFRAMESPERSECOND));
     }
#else
     if (recording) {
        se.write(cString::sprintf(" <is_pes_recording>true</is_pes_recording>\n"));
     } else {
        se.write(cString::sprintf(" <is_pes_recording>false</is_pes_recording>\n"));
     }
     se.write(cString::sprintf(" <frames_per_second>%i</frames_per_second>\n", FRAMESPERSEC));
#endif
  } else {
     string channelid = "";
     string channelname = "";  
     int channelnumber = statm->getChannel();  
     cChannel* channel = Channels.GetByNumber(channelnumber);
     cEvent* event = NULL;
     if (channel != NULL) { 
        channelid = (const char*)channel->GetChannelID().ToString();
        channelname = channel->Name();
        event = VdrExtension::getCurrentEventOnChannel(channel);  
     }

     se.write(cString::sprintf(" <channel>%s</channel>\n", channelid.c_str()));
     se.write(cString::sprintf(" <channel_name>%s</channel_name>\n", channelname.c_str()));
     se.write(cString::sprintf(" <channel_number>%d</channel_number>\n", channelnumber));
     if ( event != NULL) {
        string eventTitle = "";
        if ( event->Title() != NULL ) { eventTitle = event->Title(); }

        se.write(cString::sprintf(" <eventid>%i</eventid>\n", event->EventID()));
        se.write(cString::sprintf(" <start_time>%i</start_time>\n", (int)event->StartTime()));
        se.write(cString::sprintf(" <duration>%i</duration>\n", (int)event->Duration()));
        se.write(cString::sprintf(" <title>%s</title>\n", StringExtension::encodeToXml(eventTitle).c_str()));
     }
  }
  SerDiskSpaceInfo ds;
  ds.Description = cVideoDiskUsage::String(); //description call goes first, it calls HasChanged
  ds.UsedPercent = cVideoDiskUsage::UsedPercent();
  ds.FreeMB      = cVideoDiskUsage::FreeMB();
  ds.FreeMinutes = cVideoDiskUsage::FreeMinutes();

  se.write(" <diskusage>\n");
  se.write(cString::sprintf("  <free_mb>%i</free_mb>\n", ds.FreeMB));
  se.write(cString::sprintf("  <free_minutes>%i</free_minutes>\n", ds.FreeMinutes));
  se.write(cString::sprintf("  <used_percent>%i</used_percent>\n", ds.UsedPercent));
  se.write(cString::sprintf("  <description_localized>%s</description_localized>\n", ds.Description.c_str()));
  se.write(" </diskusage>\n");

  se.write(cString::sprintf(" <recording>%s</recording>\n", statm->isRecord() ? "true" : "false" ));  

  se.write(" <vdr>\n"); 
  se.write(cString::sprintf("  <version>%s</version>\n", VDRVERSION));
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
  si.addMember("version") <<= pl.Version;
  si.addMember("plugins") <<= pl.plugins;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerPlayerInfo& pi)
{
  si.addMember("name") <<= pi.Name;
  si.addMember("filename") <<= pi.FileName;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerDiskSpaceInfo& ds)
{
  si.addMember("free_mb") <<= ds.FreeMB;
  si.addMember("used_percent") <<= ds.UsedPercent;
  si.addMember("free_minutes") <<= ds.FreeMinutes;
  si.addMember("description_localized") <<= ds.Description;
}
