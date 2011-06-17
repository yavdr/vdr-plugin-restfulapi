#include "channels.h"

void ChannelsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }
 
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

  int channel_details = q.getParamAsInt(0);
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");

  if (channel_details > -1) {
     cChannel* channel = VdrExtension::getChannel(channel_details);
     if (channel == NULL || channel->GroupSep()) {
        reply.httpReturn(403, "The requested channel is not available.");
        delete channelList;
        return;        
     } else {
        channelList->init();
        channelList->addChannel(channel);
     }
     //channelList->setTotal(-1);
     int total = 0;
     for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
     { if (!channel->GroupSep()) total++; }
     channelList->setTotal(total);
  } else {
     if ( start_filter > 0 && limit_filter > 0 ) {
        channelList->activateLimit(start_filter, limit_filter);
     }
     channelList->init();
     int total = 0;
     for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
     {
       if (!channel->GroupSep()) {
          channelList->addChannel(channel);
          total++;
       }
     }
     channelList->setTotal(total);
  }

  channelList->finish();
  delete channelList;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerChannel& c)
{
  si.addMember("name") <<= c.Name;
  si.addMember("number") <<= c.Number;
  si.addMember("channel_id") <<= c.ChannelId;
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

void HtmlChannelList::addChannel(cChannel* channel)
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

void JsonChannelList::addChannel(cChannel* channel)
{
  if ( filtered() ) return;

  std::string suffix = (std::string) ".ts";

  SerChannel serChannel;
  serChannel.Name = StringExtension::UTF8Decode(channel->Name());
  serChannel.Number = channel->Number();
  serChannel.ChannelId = StringExtension::UTF8Decode((std::string)channel->GetChannelID().ToString());
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
  serializer.serialize(serChannels.size(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlChannelList::init()
{
  counter = 0;
  s->writeXmlHeader();
  s->write("<channels xmlns=\"http://www.domain.org/restfulapi/2011/channels-xml\">\n");
}

void XmlChannelList::addChannel(cChannel* channel)
{
  if ( filtered() ) return;

  std::string suffix = (std::string) ".ts";

  s->write(" <channel>\n");
  s->write((const char*)cString::sprintf("  <param name=\"name\">%s</param>\n", StringExtension::encodeToXml(channel->Name()).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"number\">%i</param>\n", channel->Number()));
  s->write((const char*)cString::sprintf("  <param name=\"channel_id\">%s</param>\n",  StringExtension::encodeToXml( (std::string)channel->GetChannelID().ToString()).c_str()));
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
