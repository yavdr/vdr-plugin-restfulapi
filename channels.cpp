#include "channels.h"
using namespace std;

void ChannelsResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);

  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }
  static cxxtools::Regex imageRegex("/channels/image/*");
  static cxxtools::Regex groupsRegex("/channels/groups/*");

  if ( imageRegex.match(request.url()) ) {
     replyImage(out, request, reply);
  } else if (groupsRegex.match(request.url()) ){
     replyGroups(out, request, reply);
  } else {
     replyChannels(out, request, reply);
  }
}

void ChannelsResponder::replyChannels(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/channels", request);
  
  ChannelList* channelList;

  if ( q.isFormat(".json") ) {
    reply.addHeader("Content-Type", "application/json; charset=utf-8");
    channelList = (ChannelList*)new JsonChannelList(&out);
  } else if ( q.isFormat(".html") ) {
    reply.addHeader("Content-Type", "text/html; charset=utf-8");
    channelList = (ChannelList*)new HtmlChannelList(&out);
  } else if ( q.isFormat(".xml") ) {
    reply.addHeader("Content-Type", "text/xml; charset=utf-8");
    channelList = (ChannelList*)new XmlChannelList(&out);
  } else {
    reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .html or .xml)");
    return;
  }

  string channel_details = q.getParamAsString(0);
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  string group_filter = q.getOptionAsString("group");

  if (channel_details.length() > 0) {
     cChannel* channel = VdrExtension::getChannel(channel_details);
     if (channel == NULL || channel->GroupSep()) {
        reply.httpReturn(403, "The requested channel is not available.");
        delete channelList;
        return;        
     } else {
        channelList->init();
        
        string group = "";
        int total = 0;
        for (cChannel *channelIt = Channels.First(); channelIt; channelIt = Channels.Next(channelIt))
        { 
           if (!channelIt->GroupSep()) 
              total++; 
           else
              if ( total < channel->Number())
                 group = channelIt->Name();
        }
        channelList->setTotal(total);
        string image = FileCaches::get()->searchChannelLogo(channel);
        channelList->addChannel(channel, group, image.length() == 0);
     }
  } else {
     if ( start_filter >= 0 && limit_filter >= 1 ) {
        channelList->activateLimit(start_filter, limit_filter);
     }
     channelList->init();
     int total = 0;
     string group = "";
     for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
     {
       if (!channel->GroupSep()) {
          if ( group_filter.length() == 0 || group == group_filter ) {
             string image = FileCaches::get()->searchChannelLogo(channel);
             channelList->addChannel(channel, group, image.length() != 0);
             total++;
          }
       } else {
         group = channel->Name();
       }
     }
     channelList->setTotal(total);
  }

  channelList->finish();
  delete channelList;
}

void ChannelsResponder::replyImage(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  StreamExtension se(&out);
  QueryHandler q("/channels/image/", request);
  
  string channelid = q.getParamAsString(0);
  cChannel* channel = VdrExtension::getChannel(channelid);
  string imageFolder = Settings::get()->ChannelLogoDirectory() + (string)"/";
  
  if (channel == NULL) {
     reply.httpReturn(502, "Channel not found!");
     return;
  }

  string imageName = FileCaches::get()->searchChannelLogo(channel);

  if (imageName.length() == 0 ) {
     reply.httpReturn(502, "No image found!");
     return;
  }
  
  string absolute_path = imageFolder + imageName;
  string contenttype = (string)"image/" + imageName.substr( imageName.find_last_of('.') + 1 );
  if ( se.writeBinary(absolute_path) ) {
     reply.addHeader("Content-Type", contenttype.c_str());
  } else {
    reply.httpReturn(502, "Binary Output failed");
  }
}

void ChannelsResponder::replyGroups(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{ 
  QueryHandler q("/channels/groups", request);
  ChannelGroupList* channelGroupList;
  
  if ( q.isFormat(".json") ) {
    reply.addHeader("Content-Type", "application/json; charset=utf-8");
    channelGroupList = (ChannelGroupList*)new JsonChannelGroupList(&out);
  } else if ( q.isFormat(".html") ) {
    reply.addHeader("Content-Type", "text/html; charset=utf-8");
    channelGroupList = (ChannelGroupList*)new HtmlChannelGroupList(&out);
  } else if ( q.isFormat(".xml") ) {
    reply.addHeader("Content-Type", "text/xml; charset=utf-8");
    channelGroupList = (ChannelGroupList*)new XmlChannelGroupList(&out);
  } else {
    reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .html or .xml)");
    return;
  }

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  if ( start_filter >= 0 && limit_filter >= 1 ) {
     channelGroupList->activateLimit(start_filter, limit_filter);
  }

  channelGroupList->init();
  int total = 0;
  
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
      if (channel->GroupSep()) {
         channelGroupList->addGroup((std::string)channel->Name());
         total++;
      }
  }

  channelGroupList->setTotal(total);
  channelGroupList->finish();

  delete channelGroupList;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerChannel& c)
{
  si.addMember("name") <<= c.Name;
  si.addMember("number") <<= c.Number;
  si.addMember("channel_id") <<= c.ChannelId;
  si.addMember("image") <<= c.Image;
  si.addMember("group") <<= c.Group;
  si.addMember("transponder") <<= c.Transponder;
  si.addMember("stream") <<= c.Stream;
  si.addMember("is_atsc") <<= c.IsAtsc;
  si.addMember("is_cable") <<= c.IsCable;
  si.addMember("is_terr") <<= c.IsTerr;
  si.addMember("is_sat") <<= c.IsSat;
  si.addMember("is_radio") <<= c.IsRadio;
}

ChannelList::ChannelList(ostream* _out)
{
  s = new StreamExtension(_out);
}

ChannelList::~ChannelList()
{
  delete s;
}

void HtmlChannelList::init()
{
  s->writeHtmlHeader( "HtmlChannelList" );
  s->write("<ul>");
}

void HtmlChannelList::addChannel(cChannel* channel, string group, bool image)
{
  if ( filtered() ) return;

  s->write("<li>");
  s->write((char*)channel->Name());
  s->write("\n");
}

void HtmlChannelList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonChannelList::addChannel(cChannel* channel, string group, bool image)
{  
  if ( filtered() ) return;

  string suffix = (string) ".ts";

  SerChannel serChannel;
  serChannel.Name = StringExtension::UTF8Decode(channel->Name());
  serChannel.Number = channel->Number();
  serChannel.ChannelId = StringExtension::UTF8Decode((string)channel->GetChannelID().ToString());
  serChannel.Image = image;
  serChannel.Group = StringExtension::UTF8Decode(group);
  serChannel.Transponder = channel->Transponder();
  serChannel.Stream = StringExtension::UTF8Decode(((string)channel->GetChannelID().ToString() + (string)suffix).c_str());
  // TODO: There is an atsc Patch
  #if APIVERSNUM >= 10714
  serChannel.IsAtsc = channel->IsAtsc();
  #else
  serChannel.IsAtsc = false;
  #endif
  serChannel.IsCable = channel->IsCable();
  serChannel.IsSat = channel->IsSat();
  serChannel.IsTerr = channel->IsTerr();
  serChannel.IsRadio = VdrExtension::IsRadio(channel);
  serChannels.push_back(serChannel);
}

void JsonChannelList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(serChannels, "channels");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlChannelList::init()
{
  s->writeXmlHeader();
  s->write("<channels xmlns=\"http://www.domain.org/restfulapi/2011/channels-xml\">\n");
}

void XmlChannelList::addChannel(cChannel* channel, string group, bool image)
{
  if ( filtered() ) return;

  string suffix = (string) ".ts";

  s->write(" <channel>\n");
  s->write(cString::sprintf("  <param name=\"name\">%s</param>\n", StringExtension::encodeToXml(channel->Name()).c_str()));
  s->write(cString::sprintf("  <param name=\"number\">%i</param>\n", channel->Number()));
  s->write(cString::sprintf("  <param name=\"channel_id\">%s</param>\n",  StringExtension::encodeToXml( (string)channel->GetChannelID().ToString()).c_str()));
  s->write(cString::sprintf("  <param name=\"image\">%s</param>\n", (image ? "true" : "false")));
  s->write(cString::sprintf("  <param name=\"group\">%s</param>\n", StringExtension::encodeToXml( group ).c_str()));
  s->write(cString::sprintf("  <param name=\"transponder\">%i</param>\n", channel->Transponder()));
  s->write(cString::sprintf("  <param name=\"stream\">%s</param>\n", StringExtension::encodeToXml( ((string)channel->GetChannelID().ToString() + (string)suffix).c_str()).c_str()));
  // TODO: There is an atsc Patch
  #if APIVERSNUM >= 10714
  s->write(cString::sprintf("  <param name=\"is_atsc\">%s</param>\n", channel->IsAtsc() ? "true" : "false"));
  #else
  s->write(cString::sprintf("  <param name=\"is_atsc\">%s</param>\n", false ? "true" : "false"));
  #endif
  s->write(cString::sprintf("  <param name=\"is_cable\">%s</param>\n", channel->IsCable() ? "true" : "false"));
  s->write(cString::sprintf("  <param name=\"is_sat\">%s</param>\n", channel->IsSat() ? "true" : "false"));
  s->write(cString::sprintf("  <param name=\"is_terr\">%s</param>\n", channel->IsTerr() ? "true" : "false"));
  bool is_radio = VdrExtension::IsRadio(channel);
  s->write(cString::sprintf("  <param name=\"is_radio\">%s</param>\n", is_radio ? "true" : "false"));
  s->write(" </channel>\n");
}

void XmlChannelList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</channels>");
}

ChannelGroupList::ChannelGroupList(std::ostream* _out) 
{
  s = new StreamExtension(_out);
}

ChannelGroupList::~ChannelGroupList()
{
  delete s;
}

void HtmlChannelGroupList::init()
{
  s->writeHtmlHeader( "HtmlChannelGroupList" );
  s->write("<ul>");
}

void HtmlChannelGroupList::addGroup(string group)
{
  if ( filtered() ) return;

  s->write("<li>");
  s->write(group.c_str());
  s->write("\n");
}

void HtmlChannelGroupList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonChannelGroupList::addGroup(string group)
{
  if ( filtered() ) return;
  groups.push_back(StringExtension::UTF8Decode(group));
}

void JsonChannelGroupList::finish()
{  
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(groups, "groups");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlChannelGroupList::init()
{
  s->writeXmlHeader();
  s->write("<groups xmlns=\"http://www.domain.org/restfulapi/2011/groups-xml\">\n");
}

void XmlChannelGroupList::addGroup(string group)
{
  if ( filtered() ) return;
  s->write(cString::sprintf(" <group>%s</group>\n", group.c_str()));
}

void XmlChannelGroupList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</groups>");
}
