#include "channels.h"

void ChannelsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
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

void ChannelsResponder::replyChannels(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
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

  bool scan_images = false; 
  if ( q.getOptionAsString("images") == "true" ) {
     scan_images = true;
  }

  int channel_details = q.getParamAsInt(0);
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  std::string group_filter = q.getOptionAsString("group");

  if (channel_details > -1) {
     cChannel* channel = VdrExtension::getChannel(channel_details);
     if (channel == NULL || channel->GroupSep()) {
        reply.httpReturn(403, "The requested channel is not available.");
        delete channelList;
        return;        
     } else {
        channelList->init();
        
        std::string group = "";
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
        std::string image = scan_images ? VdrExtension::getChannelImage(channel) : "";
        channelList->addChannel(channel, group, image);
     }
  } else {
     if ( start_filter > 0 && limit_filter > 0 ) {
        channelList->activateLimit(start_filter, limit_filter);
     }
     channelList->init();
     int total = 0;
     std::string group = "";
     for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
     {
       if (!channel->GroupSep()) {
          if ( group_filter.length() == 0 || group == group_filter ) {
             std::string image = scan_images ? VdrExtension::getChannelImage(channel) : "";
             channelList->addChannel(channel, group, image);
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

void ChannelsResponder::replyImage(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  StreamExtension se(&out);
  QueryHandler q("/events/image/", request);
  
  std::string imageName = StringExtension::replace(StringExtension::toLowerCase(q.getParamAsString(0)), " ", "_");
  std::string imageFolder = "/usr/share/vdr/channel-logos/";
  std::string folderWildcard = imageFolder + (std::string)"*";
 
  //check if requestes imagepath points to a file in the imageFolder directory - security check
  if ( VdrExtension::doesFileExistInFolder(folderWildcard, imageName) ) {
    std::string absolute_path = imageFolder + imageName;
    std::string contenttype = (std::string)"image/" + imageName.substr( imageName.find_last_of('.') + 1 );
    if ( se.writeBinary(absolute_path) ) {
       reply.addHeader("Content-Type", contenttype.c_str());
    } else {
       reply.httpReturn(502, "Binary Output failed");
    }
  } else {
    reply.httpReturn(403, "Please learn to crack before using useless tools on my absolutely secure web service. You can only retrieve channel-logos, trying to retrieve files like the ones of passwd is a waste of time - it won't work!");
  }

}

void ChannelsResponder::replyGroups(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
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
  if ( start_filter >= 1 && limit_filter >= 1 ) {
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
}

void operator<<= (cxxtools::SerializationInfo& si, const SerChannels& c)
{
  si.addMember("channels") <<= c.channel;
}

ChannelList::ChannelList(std::ostream* _out)
{
  s = new StreamExtension(_out);
}

ChannelList::~ChannelList()
{
  delete s;
}

void HtmlChannelList::init()
{
  s->writeHtmlHeader();
  s->write("<ul>");
}

void HtmlChannelList::addChannel(cChannel* channel, std::string group, std::string image)
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

void JsonChannelList::addChannel(cChannel* channel, std::string group, std::string image)
{  
  if ( filtered() ) return;

  std::string suffix = (std::string) ".ts";

  SerChannel serChannel;
  serChannel.Name = StringExtension::UTF8Decode(channel->Name());
  serChannel.Number = channel->Number();
  serChannel.ChannelId = StringExtension::UTF8Decode((std::string)channel->GetChannelID().ToString());
  serChannel.Image = StringExtension::UTF8Decode(image);
  serChannel.Group = StringExtension::UTF8Decode(group);
  serChannel.Transponder = channel->Transponder();
  serChannel.Stream = StringExtension::UTF8Decode(((std::string)channel->GetChannelID().ToString() + (std::string)suffix).c_str());
  serChannel.IsAtsc = channel->IsAtsc();
  serChannel.IsCable = channel->IsCable();
  serChannel.IsSat = channel->IsSat();
  serChannel.IsTerr = channel->IsTerr();
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

void XmlChannelList::addChannel(cChannel* channel, std::string group, std::string image)
{
  if ( filtered() ) return;

  std::string suffix = (std::string) ".ts";

  s->write(" <channel>\n");
  s->write((const char*)cString::sprintf("  <param name=\"name\">%s</param>\n", StringExtension::encodeToXml(channel->Name()).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"number\">%i</param>\n", channel->Number()));
  s->write((const char*)cString::sprintf("  <param name=\"channel_id\">%s</param>\n",  StringExtension::encodeToXml( (std::string)channel->GetChannelID().ToString()).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"image\">%s</param>\n", StringExtension::encodeToXml( image ).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"group\">%s</param>\n", StringExtension::encodeToXml( group ).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"transponder\">%i</param>\n", channel->Transponder()));
  s->write((const char*)cString::sprintf("  <param name=\"stream\">%s</param>\n", StringExtension::encodeToXml( ((std::string)channel->GetChannelID().ToString() + (std::string)suffix).c_str()).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"is_atsc\">%s</param>\n", channel->IsAtsc() ? "true" : "false"));
  s->write((const char*)cString::sprintf("  <param name=\"is_cable\">%s</param>\n", channel->IsCable() ? "true" : "false"));
  s->write((const char*)cString::sprintf("  <param name=\"is_sat\">%s</param>\n", channel->IsSat() ? "true" : "false"));
  s->write((const char*)cString::sprintf("  <param name=\"is_terr\">%s</param>\n", channel->IsTerr() ? "true" : "false"));
  s->write(" </channel>\n");
}

void XmlChannelList::finish()
{
  s->write((const char*)cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
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
  s->writeHtmlHeader();
  s->write("<ul>");
}

void HtmlChannelGroupList::addGroup(std::string group)
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

void JsonChannelGroupList::addGroup(std::string group)
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

void XmlChannelGroupList::addGroup(std::string group)
{
  if ( filtered() ) return;
  s->write((const char*)cString::sprintf(" <group>%s</group>\n", group.c_str()));
}

void XmlChannelGroupList::finish()
{
  s->write((const char*)cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</groups>");
}
