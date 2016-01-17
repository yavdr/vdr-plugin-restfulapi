#include "info.h"
#include <vdr/videodir.h>
using namespace std;

void InfoResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  QueryHandler q("/info", request);

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET");
      reply.httpReturn(200, "OK");
      return;
  }

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
  if ( !se.writeBinary(DOCUMENT_ROOT "API.html") ) {
     se.write("Copy API.html to " DOCUMENT_ROOT);
  }
}

void InfoResponder::replyJson(StreamExtension& se)
{
  time_t now = time(0);
  StatusMonitor* statm = StatusMonitor::get();

  cxxtools::JsonSerializer serializer(*se.getBasicStream());
  serializer.serialize(cPluginManager::GetPlugin("restfulapi")->Version(), "version");
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

  struct SerVDR vdr;

  cPlugin* p = NULL;
  int counter = 0;
  while ( (p = cPluginManager::GetPlugin(counter)) != NULL ) {
     struct SerPlugin sp;
     sp.Name = StringExtension::UTF8Decode(p->Name());
     sp.Version = StringExtension::UTF8Decode(p->Version());
     vdr.plugins.push_back(sp);
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
     const cChannel* channel = VdrExtension::getChannel(statm->getChannel());
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

  SerDiskSpaceInfo ds;
  ds.Description = cVideoDiskUsage::String(); //description call goes first, it calls HasChanged
  ds.UsedPercent = cVideoDiskUsage::UsedPercent();
  ds.FreeMB      = cVideoDiskUsage::FreeMB();
  ds.FreeMinutes = cVideoDiskUsage::FreeMinutes();
  serializer.serialize(ds, "diskusage");

  int numDevices = cDevice::NumDevices();
  int i;
  for (i=0; i<numDevices;i++) {
      SerDevice sd = getDeviceSerializeInfo(i);
      vdr.devices.push_back(sd);
  }

  serializer.serialize(vdr, "vdr");

  serializer.finish();  
}

void InfoResponder::replyXml(StreamExtension& se)
{
  time_t now = time(0);
  StatusMonitor* statm = StatusMonitor::get();


  se.writeXmlHeader();
  se.write("<info xmlns=\"http://www.domain.org/restfulapi/2011/info-xml\">\n");
  se.write(cString::sprintf(" <version>%s</version>\n", cPluginManager::GetPlugin("restfulapi")->Version()));
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
     const cChannel* channel = VdrExtension::getChannel(statm->getChannel());

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

  se.write(" <vdr>\n");
  se.write(cString::sprintf(" <version>%s</version>\n", VDRVERSION));
  se.write("  <plugins>\n");
 
  cPlugin* p = NULL; 
  int counter = 0;
  while ( (p = cPluginManager::GetPlugin(counter) ) != NULL ) {
     se.write(cString::sprintf("   <plugin name=\"%s\" version=\"%s\" />\n", p->Name(), p->Version()));
     counter++;
  }

  se.write("  </plugins>\n");
  se.write(" </vdr>\n");
  se.write(" <devices>");

  int numDevices = cDevice::NumDevices();
  int i;
  for (i=0; i<numDevices;i++) {
      SerDevice sd = getDeviceSerializeInfo(i);
      se.write(" <device>");
      se.write(cString::sprintf("    <dvb_c>%s</dvb_c>\n", (sd.dvbc ? "true" : "false")));
      se.write(cString::sprintf("    <dvb_s>%s</dvb_s>\n", (sd.dvbs ? "true" : "false")));
      se.write(cString::sprintf("    <dvb_t>%s</dvb_t>\n", (sd.dvbt ? "true" : "false")));
      se.write(cString::sprintf("    <atsc>%s</atsc>\n", (sd.atsc ? "true" : "false")));
      se.write(cString::sprintf("    <primary>%s</primary>\n", (sd.primary ? "true" : "false")));
      se.write(cString::sprintf("    <has_decoder>%s</has_decoder>\n", (sd.hasDecoder ? "true" : "false")));
      se.write(cString::sprintf("    <name>%s</name>\n", StringExtension::encodeToXml(sd.Name).c_str()));
      se.write(cString::sprintf("    <number>%i</number>\n", sd.Number));
      se.write(cString::sprintf("    <channel_id>%s</channel_id>\n", StringExtension::encodeToXml(sd.ChannelId).c_str()));
      se.write(cString::sprintf("    <channel_name>%s</channel_name>\n", StringExtension::encodeToXml(sd.ChannelName).c_str()));
      se.write(cString::sprintf("    <channel_nr>%i</channel_nr>\n", sd.ChannelNr));
      se.write(cString::sprintf("    <live>%s</live>\n", (sd.Live ? "true" : "false")));
      se.write(cString::sprintf("    <has_ci>%s</has_ci>\n", (sd.HasCi ? "true" : "false")));
      se.write(cString::sprintf("    <signal_strength>%i</signal_strength>\n", sd.SignalStrength));
      se.write(cString::sprintf("    <signal_quality>%i</signal_quality>\n", sd.SignalQuality));

      se.write(cString::sprintf("    <signal_str>%i</signal_str>\n", sd.str));
      se.write(cString::sprintf("    <signal_snr>%i</signal_snr>\n", sd.snr));
      se.write(cString::sprintf("    <signal_ber>%i</signal_ber>\n", sd.ber));
      se.write(cString::sprintf("    <signal_unc>%i</signal_unc>\n", sd.unc));
      se.write(cString::sprintf("    <signal_status>%s</signal_status>\n", StringExtension::encodeToXml(sd.status).c_str()));


      se.write(cString::sprintf("    <adapter>%i</adapter>\n", sd.Adapter));
      se.write(cString::sprintf("    <frontend>%i</frontend>\n", sd.Frontend));
      se.write(cString::sprintf("    <type>%s</type>\n", StringExtension::encodeToXml(sd.Type).c_str()));
      se.write(" </device>");
  }
  se.write(" </devices>");

  se.write("</info>");
}

SerDevice InfoResponder::getDeviceSerializeInfo(int index) {

  SerDevice sd;
  cDvbDevice* dev = VdrExtension::getDevice(index);
  cString deviceName = dev->DeviceName();
  const char * name = deviceName;
  const cChannel * chan = dev->GetCurrentlyTunedTransponder();
  string channelName = "n.a.";
  string channelId = "n.a.";
  bool hasCi = false;

  int signalStrength = -1;
  int signalQuality = -1;
  uint16_t str = 0;
  uint16_t snr = 0;
  uint32_t ber = 0;
  uint32_t unc = 0;
  fe_status_t statusValue;
  char status[50];
  int adapter = -1;
  int frontend = -1;

  string type = "n.a.";
  int channelNr = -1;
  bool live = false;

  if (dev->ProvidesSource(cSource::stCable) || dev->ProvidesSource(cSource::stSat) || dev->ProvidesSource(cSource::stTerr) || dev->ProvidesSource(cSource::stAtsc)) {
	  if (chan) {
		  channelName = (string)chan->Name();
		  channelId = (string)chan->GetChannelID().ToString();
		  channelNr = chan->Number();
		  live = channelNr == StatusMonitor::get()->getChannel();
	  }

      hasCi = dev->HasCi();
      signalStrength = dev->SignalStrength();
      signalQuality = dev->SignalQuality();
      adapter = dev->Adapter();
      frontend = dev->Frontend();
      type = (string)dev->DeviceType();

      int fe = open(*cString::sprintf(FRONTEND_DEVICE, dev->Adapter(), dev->Frontend()), O_RDONLY | O_NONBLOCK);
      if (fe >= 0) {
    	  ioctl(fe, FE_READ_SIGNAL_STRENGTH, &str);
    	  ioctl(fe, FE_READ_SNR, &snr);
    	  ioctl(fe, FE_READ_BER, &ber);
    	  ioctl(fe, FE_READ_UNCORRECTED_BLOCKS, &unc);

    	  memset(&statusValue, 0, sizeof(statusValue));
    	  ioctl(fe, FE_READ_STATUS, &statusValue);
    	  sprintf(
			status,
			"%s:%s:%s:%s:%s",
			(statusValue & FE_HAS_LOCK) ? "LOCKED" : "-",
			(statusValue & FE_HAS_SIGNAL) ? "SIGNAL" : "-",
			(statusValue & FE_HAS_CARRIER) ? "CARRIER" : "-",
			(statusValue & FE_HAS_VITERBI) ? "VITERBI" : "-",
			(statusValue & FE_HAS_SYNC) ? "SYNC" : "-"
    	);
      }
      close(fe);
  }

  sd.dvbc = dev->ProvidesSource(cSource::stCable);
  sd.dvbs = dev->ProvidesSource(cSource::stSat);
  sd.dvbt = dev->ProvidesSource(cSource::stTerr);
  sd.atsc = dev->ProvidesSource(cSource::stAtsc);
  sd.primary = dev->IsPrimaryDevice();
  sd.hasDecoder = dev->HasDecoder();
  sd.Name = name;
  sd.Number = dev->CardIndex();
  sd.ChannelId = StringExtension::UTF8Decode(channelId);
  sd.ChannelName = StringExtension::UTF8Decode(channelName);
  sd.ChannelNr = channelNr;
  sd.Live = live;
  sd.HasCi = hasCi;
  sd.SignalStrength = signalStrength;
  sd.SignalQuality = signalQuality;
  sd.str = str;
  sd.snr = snr;
  sd.ber = ber;
  sd.unc = unc;
  sd.status = status;
  sd.Adapter = adapter;
  sd.Frontend = frontend;
  sd.Type = type;

  return sd;
};

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

void operator<<= (cxxtools::SerializationInfo& si, const SerVDR& vdr)
{
  si.addMember("version") <<= VDRVERSION;
  si.addMember("plugins") <<= vdr.plugins;
  si.addMember("devices") <<= vdr.devices;
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

void operator<<= (cxxtools::SerializationInfo& si, const SerDevice& d)
{
  si.addMember("name") <<= d.Name;
  si.addMember("dvb_c") <<= d.dvbc;
  si.addMember("dvb_s") <<= d.dvbs;
  si.addMember("dvb_t") <<= d.dvbt;
  si.addMember("atsc") <<= d.atsc;
  si.addMember("primary") <<= d.primary;
  si.addMember("has_decoder") <<= d.hasDecoder;
  si.addMember("number") <<= d.Number;
  si.addMember("channel_id") <<= d.ChannelId;
  si.addMember("channel_name") <<= d.ChannelName;
  si.addMember("channel_nr") <<= d.ChannelNr;
  si.addMember("live") <<= d.Live;
  si.addMember("has_ci") <<= d.HasCi;
  si.addMember("signal_strength") <<= d.SignalStrength;
  si.addMember("signal_quality") <<= d.SignalQuality;
  si.addMember("str") <<= d.str;
  si.addMember("snr") <<= d.snr;
  si.addMember("ber") <<= d.ber;
  si.addMember("unc") <<= d.unc;
  si.addMember("status") <<= d.status;
  si.addMember("adapter") <<= d.Adapter;
  si.addMember("frontend") <<= d.Frontend;
  si.addMember("type") <<= d.Type;
}
