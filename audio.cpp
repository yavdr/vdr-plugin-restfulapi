#include "audio.h"
using namespace std;

void AudioResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  
  if (request.method() == "POST") { 
     QueryHandler q("/audio", request);
     string vol = q.getOptionAsString("volume");
     int level = q.getOptionAsInt("volume");
     int mute = q.getOptionAsInt("mute");
     int track = q.getOptionAsInt("track");
     int channel = q.getOptionAsInt("channel");

     if (vol.find("m") == 0) {
        cDevice::PrimaryDevice()->ToggleMute();
        mute = -1;
     } else if (vol.find("0") == 0) {
        cDevice::PrimaryDevice()->SetVolume(level, false);
     } else if (vol.find("-") == 0) {
       cDevice::PrimaryDevice()->SetVolume(level, false);
     } else if ( level >= 0 && level <= 255 ) {
       cDevice::PrimaryDevice()->SetVolume(level, true);
     }

     if (mute >= 0) {
        if (mute == 2) {
           cDevice::PrimaryDevice()->ToggleMute();
        } else if (cDevice::PrimaryDevice()->IsMute()) {
           if (mute == 0)
              cDevice::PrimaryDevice()->ToggleMute();
        } else {
           if (mute == 1)
              cDevice::PrimaryDevice()->ToggleMute();
        }
     }

     if (track >= 0) {
        const tTrackId *TrackId = cDevice::PrimaryDevice()->GetTrack(eTrackType(track));
        if (TrackId && TrackId->id)
           cDevice::PrimaryDevice()->SetCurrentAudioTrack(eTrackType(track));
     }

     if (channel >= 0 && channel < 3) {
        cDevice::PrimaryDevice()->SetAudioChannel(channel);
     }

     AudioList* audioList;
     if ( q.isFormat(".html") ) {
        reply.addHeader("Content-Type", "text/html; charset=utf-8");
        audioList = (AudioList*)new HtmlAudioList(&out);
        audioList->init();   
     } else if ( q.isFormat(".xml") ) {
        reply.addHeader("Content-Type", "text/xml; charset=utf-8");
        audioList = (AudioList*)new XmlAudioList(&out);
        audioList->init();  
     } else { //  if ( q.isFormat(".json") ) 
        reply.addHeader("Content-Type", "application/json; charset=utf-8");
        audioList = (AudioList*)new JsonAudioList(&out);
     }
     audioList->addContent();
     audioList->finish();
     delete audioList;

  } else {
     reply.httpReturn(403, "Only POST method is supported by the audio control");
  }
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTrack& t)
{
  si.addMember("type") <<= t.Number;
  si.addMember("description") <<= t.Description;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTrackList& tl)
{
  si.addMember("count") <<= tl.Count;
  si.addMember("track") <<= tl.track;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerAudio& a)
{
  si.addMember("volume") <<= a.Volume;
  si.addMember("mute") <<= a.Mute;
  si.addMember("tracks") <<= a.Tracks;
  si.addMember("type") <<= a.Number;
  si.addMember("description") <<= a.Description;
  si.addMember("channel") <<= a.Channel;
}

AudioList::AudioList(ostream *out)
{
  s = new StreamExtension(out);
}

AudioList::~AudioList()
{
  delete s;
}

void HtmlAudioList::init()
{
  s->writeHtmlHeader("HtmlAudioList");
}

void HtmlAudioList::addContent()
{
  cDevice *Device = cDevice::PrimaryDevice();
  eTrackType currentTrack = Device->GetCurrentAudioTrack();  
  const char *desc;
  
  s->write(cString::sprintf("volume: %d<br>\n", Device->CurrentVolume()));
  if (Device->IsMute())
     s->write(cString::sprintf("mute: 1<br>\n"));
  else
     s->write(cString::sprintf("mute: 0<br>\n"));

  s->write(cString::sprintf("tracks: %i<br>\n", Device->NumAudioTracks())); 
  for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
      const tTrackId *TrackId = Device->GetTrack(eTrackType(i));
      if (TrackId && TrackId->id) {
         s->write(cString::sprintf("<li> track: type=%i description=%s </li>\n", i, *TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i)));
         if (i == currentTrack)
            desc = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i)); 
      }
  }  
 
  s->write(cString::sprintf("type: %i<br>\n", currentTrack));
  s->write(cString::sprintf("description: %s<br>\n", desc));

  if (IS_DOLBY_TRACK(currentTrack)) { // strncmp (Track->description, "Dolby", 5) == 0
     if (strstr (desc, "5.1") == 0)
        s->write(cString::sprintf("channel: dd 2.0<br>\n"));
     else
        s->write(cString::sprintf("channel: dd 5.1<br>\n"));  
  } else {
     s->write(cString::sprintf("channel: %i<br>\n", Device->GetAudioChannel())); 
  }

  s->write("\n");
}

void HtmlAudioList::finish()
{
  s->write("</body></html>");
}

void JsonAudioList::addContent()
{
  cDevice *Device = cDevice::PrimaryDevice();
  eTrackType currentTrack = Device->GetCurrentAudioTrack();
  const char *desc;

  SerAudio serAudio;  
  serAudio.Volume = Device->CurrentVolume();
  if (Device->IsMute())
     serAudio.Mute = 1;
  else
     serAudio.Mute = 0;
 
  struct SerTrackList tl;
  tl.Count = Device->NumAudioTracks();
  for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
      const tTrackId *TrackId = Device->GetTrack(eTrackType(i));
      if (TrackId && TrackId->id) {
         struct SerTrack t;
         t.Number = i;
         t.Description = StringExtension::UTF8Decode(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i)).c_str();
         tl.track.push_back(t);  
         if (i == currentTrack)
            desc = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i)); 
      }
  } 
  serAudio.Tracks = tl;
  serAudio.Number = currentTrack;
  serAudio.Description = StringExtension::UTF8Decode(desc);

  if (IS_DOLBY_TRACK(currentTrack)) {
     if (strstr (desc, "5.1") == 0)
        serAudio.Channel = StringExtension::UTF8Decode("dd 2.0");
     else
        serAudio.Channel = StringExtension::UTF8Decode("dd 5.1");  
  } else {
     int c = Device->GetAudioChannel();
     if (c == 0)
        serAudio.Channel = StringExtension::UTF8Decode("stereo"); 
     else if (c == 1)
        serAudio.Channel = StringExtension::UTF8Decode("mono left"); 
     else if (c == 2)
        serAudio.Channel = StringExtension::UTF8Decode("mono right"); 
  }

  serAudios.push_back(serAudio);
}

void JsonAudioList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(serAudios, "audio");
  serializer.finish();
}

void XmlAudioList::init()
{
  s->writeXmlHeader();
  s->write("<audio xmlns=\"http://www.domain.org/restfulapi/2011/audio-xml\">\n");
}

void XmlAudioList::addContent()
{
  cDevice *Device = cDevice::PrimaryDevice();
  eTrackType currentTrack = Device->GetCurrentAudioTrack();  
  const char *desc;
  
  s->write(cString::sprintf("  <volume>%d</volume>\n", Device->CurrentVolume()));
  if (Device->IsMute())
     s->write(cString::sprintf("  <mute>1</mute>\n"));
  else
     s->write(cString::sprintf("  <mute>0</mute>\n"));
 
  s->write(cString::sprintf("  <tracks count=\"%i\">\n", Device->NumAudioTracks()));
  for (int i = ttAudioFirst; i <= ttDolbyLast; i++) {
      const tTrackId *TrackId = Device->GetTrack(eTrackType(i));
      if (TrackId && TrackId->id) {
         s->write(cString::sprintf("    <track type=\"%i\" description=\"%s\" />\n", i, StringExtension::encodeToXml(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i)).c_str()));
         if (i == currentTrack)
            desc = strdup(*TrackId->description ? TrackId->description : *TrackId->language ? TrackId->language : *itoa(i)); 
      }
  }  
  s->write(cString::sprintf("  </tracks>\n"));

  s->write(cString::sprintf("  <type>%i</type>\n", currentTrack));  
  s->write(cString::sprintf("  <description>%s</description>\n", StringExtension::encodeToXml(desc).c_str()));

  if (IS_DOLBY_TRACK(currentTrack)) {
     if (strstr (desc, "5.1") == 0)
        s->write(cString::sprintf("  <channel>dd 2.0</channel>\n"));
     else
        s->write(cString::sprintf("  <channel>dd 5.1</channel>\n"));  
  } else {
     int c = Device->GetAudioChannel();
     if (c == 0)
        s->write(cString::sprintf("  <channel>stereo</channel>\n"));
     else if (c == 1)
        s->write(cString::sprintf("  <channel>mono left</channel>\n"));
     else if (c == 2)
        s->write(cString::sprintf("  <channel>mono right</channel>\n"));
  }
}

void XmlAudioList::finish()
{
  s->write("</audio>");
}
