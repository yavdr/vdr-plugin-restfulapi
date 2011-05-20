#include "events.h"

void EventsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/events", request.url()); 
  EventList* eventList;

  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  int channel_number = getIntParam(params, 0);
  int timespan = getIntParam(params, 1);
  int from = getIntParam(params, 2);
  
  cChannel* channel = getChannel(channel_number);
  if ( channel == NULL ) { 
     reply.httpReturn(404, "Channel with number _xx_ not found!"); 
     return; 
  }

  if ( from == -1 ) from = time(NULL); // default time is now
  if ( timespan == -1 ) timespan = 3600; // default timespan is one hour
  int to = from + timespan;

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);

  if( !Schedules ) {
     reply.httpReturn(404, "Could not find schedules!");
     return;
  }

  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
  
  if ( !Schedule ) {
     reply.httpReturn(404, "Could not find schedule!");
     return;
  }

  if ( isFormat(params, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     eventList = (EventList*)new JsonEventList(&out);
  } else if ( isFormat(params, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     eventList = (EventList*)new HtmlEventList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  for(cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event))
  {
    int ts = event->StartTime();
    int te = ts + event->Duration();
    if ( ts <= to && te >= from ) {
       eventList->addEvent(event);
    }else{
      if(ts > to) break;
    }
  }

  if ( isFormat(params, ".json") ) {
     delete (JsonEventList*)eventList;
  } else  {
     delete (HtmlEventList*)eventList;
  }  
}

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e)
{
  si.addMember("id") <<= e.Id;
  si.addMember("title") <<= e.Title;
  si.addMember("short_text") <<= e.ShortText;
  si.addMember("description") <<= e.Description;
  si.addMember("start_time") <<= e.StartTime;
  si.addMember("duration") <<= e.Duration;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerEvents& e)
{
  si.addMember("rows") <<= e.event;
}

HtmlEventList::HtmlEventList(std::ostream* _out) : EventList(_out)
{
  writeHtmlHeader(out);
  
  write(out, "<ul>");
}

HtmlEventList::~HtmlEventList()
{
  write(out, "</ul>");
  write(out, "</body></html>");
}

void HtmlEventList::addEvent(cEvent* event)
{
  write(out, "<li>");
  write(out, (char*)event->Title()); //TODO: add more infos
  write(out, "\n");
}

JsonEventList::JsonEventList(std::ostream* _out) : EventList(_out)
{

}

JsonEventList::~JsonEventList()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serEvents, "events");
  serializer.finish();
}

void JsonEventList::addEvent(cEvent* event)
{
  cxxtools::String eventTitle;
  cxxtools::String eventShortText;
  cxxtools::String eventDescription;
  cxxtools::String empty = UTF8Decode("");
  SerEvent serEvent;

  if( !event->Title() ) { eventTitle = empty; } else { eventTitle = UTF8Decode(event->Title()); }
  if( !event->ShortText() ) { eventShortText = empty; } else { eventShortText = UTF8Decode(event->ShortText()); }
  if( !event->Description() ) { eventDescription = empty; } else { eventDescription = UTF8Decode(event->Description()); }

  serEvent.Id = event->EventID();
  serEvent.Title = eventTitle;
  serEvent.ShortText = eventShortText;
  serEvent.Description = eventDescription;
  serEvent.StartTime = event->StartTime();
  serEvent.Duration = event->Duration();
  serEvents.push_back(serEvent);
	}
