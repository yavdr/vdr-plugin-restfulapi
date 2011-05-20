#include "channels.h"

void ChannelsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  std::string params = getRestParams((std::string)"/channels", request.url()); 
  ChannelList* channelList;

  if ( isFormat(params, ".json") ) {
    reply.addHeader("Content-Type", "application/json; charset=utf-8");
    channelList = (ChannelList*)new JsonChannelList(&out);
  } else if ( isFormat(params, ".html") ) {
    reply.addHeader("Content-Type", "text/html; charset=utf-8");
    channelList = (ChannelList*)new HtmlChannelList(&out);
  } else {
    reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json or .html)");
    return;
  }

  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
    if (!channel->GroupSep()) {
       channelList->addChannel(channel);
    }
  }

  channelList->finish();
  delete channelList;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerChannel& c)
{
  si.addMember("name") <<= c.Name;
  si.addMember("number") <<= c.Number;
  si.addMember("transponder") <<= c.Transponder;
  si.addMember("stream") <<= c.Stream;
  si.addMember("is_atsc") <<= c.IsAtsc;
  si.addMember("is_cable") <<= c.IsCable;
  si.addMember("is_terr") <<= c.IsTerr;
  si.addMember("is_sat") <<= c.IsSat;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerChannels& c)
{
  si.addMember("rows") <<= c.channel;
}

HtmlChannelList::HtmlChannelList(std::ostream* _out) : ChannelList(_out)
{
  writeHtmlHeader(out);

  write(out, "<ul>");
}

void HtmlChannelList::addChannel(cChannel* channel)
{
  write(out, "<li>");
  write(out, (char*)channel->Name());
  write(out, "\n");
}

void HtmlChannelList::finish()
{
  write(out, "</ul>");
  write(out, "</body></html>");
}

JsonChannelList::JsonChannelList(std::ostream* _out) : ChannelList(_out)
{

}

void JsonChannelList::addChannel(cChannel* channel)
{
  std::string suffix = (std::string) ".ts";

  SerChannel serChannel;
  serChannel.Name = UTF8Decode(channel->Name());
  serChannel.Number = channel->Number();
  serChannel.Transponder = channel->Transponder();
  serChannel.Stream = UTF8Decode(((std::string)channel->GetChannelID().ToString() + (std::string)suffix).c_str());
  serChannel.IsAtsc = channel->IsAtsc();
  serChannel.IsCable = channel->IsCable();
  serChannel.IsSat = channel->IsSat();
  serChannel.IsTerr = channel->IsTerr();
  serChannels.push_back(serChannel);
}

void JsonChannelList::finish()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serChannels, "channels");
  serializer.finish();
}

