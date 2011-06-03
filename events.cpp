#include "events.h"

void EventsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  if ( (int)request.url().find("/events/image/") == 0 ) {
     replyImage(out, request, reply);
  } else {
     replyEvents(out, request, reply);
  }
}

void EventsResponder::replyEvents(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/events", request.url()); 
  EventList* eventList;

  if ( isFormat(params, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     eventList = (EventList*)new JsonEventList(&out);
  } else if ( isFormat(params, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     eventList = (EventList*)new HtmlEventList(&out);
  } else if ( isFormat(params, ".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     eventList = (EventList*)new XmlEventList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  int channel_number = getIntParam(params, 0);
  int timespan = getIntParam(params, 1);
  int from = getIntParam(params, 2);

  bool scan_images = (int)request.qparams().find("images=true") != -1 ? true : false;
  
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

  eventList->init();

  for(cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event))
  {
    int ts = event->StartTime();
    int te = ts + event->Duration();
    if ( ts <= to && te >= from ) {
       eventList->addEvent(event, scan_images);
    }else{
      if(ts > to) break;
    }
  }

  eventList->finish();
  delete eventList;
}

void EventsResponder::replyImage(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  int delim = request.url().find_last_of('/');
  std::string image_filename = request.url().substr(delim + 1);
 
  cxxtools::Regex* regex1 = new cxxtools::Regex("[0-9]*_?[0-9]*.jpg");
  cxxtools::Regex* regex2 = new cxxtools::Regex("[0-9]*_?[0-9]*.png");

  if ( regex1->match(image_filename.c_str()) ) {
    reply.addHeader("Content-Type", "image/jpg");  
  } else if (regex2->match(image_filename.c_str())) {
    reply.addHeader("Content-Type", "image/png");
  } else {
    delete regex1;
    delete regex2;
    reply.httpReturn(403, "You can only download images from the specific tvm2vdr-folder!");
    return;
  }

  delete regex1;
  delete regex2;

  std::string absolute_path = (std::string)"/var/cache/vdr/epgimages/" + image_filename;

  std::ifstream* in = new std::ifstream(absolute_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

  if ( in->is_open() ) {
     
     int size = in->tellg();
     char* memory = new char[size];
     in->seekg(0, std::ios::beg);
     in->read(memory, size);
     out.write(memory, size);
     delete[] memory;
  } else {
     reply.httpReturn(404, "Could not find image!");
  }
 
  in->close();
  delete in;

}

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e)
{
  si.addMember("id") <<= e.Id;
  si.addMember("title") <<= e.Title;
  si.addMember("short_text") <<= e.ShortText;
  si.addMember("description") <<= e.Description;
  si.addMember("start_time") <<= e.StartTime;
  si.addMember("duration") <<= e.Duration;
  if (e.Images != NULL) {
      std::vector< cxxtools::String > imgs;
     for(int i=0;i<e.ImagesCount;i++) {
        imgs.push_back(e.Images[i]);
     }
     si.addMember("images") <<= imgs;
     delete[] e.Images;
  }
}

void operator<<= (cxxtools::SerializationInfo& si, const SerEvents& e)
{
  si.addMember("rows") <<= e.event;
}

void HtmlEventList::init()
{
  writeHtmlHeader(out);
  
  write(out, "<ul>");
}

void HtmlEventList::addEvent(cEvent* event, bool scan_images = false)
{
  write(out, "<li>");
  write(out, (char*)event->Title()); //TODO: add more infos
  write(out, "\n");
}

void HtmlEventList::finish()
{
  write(out, "</ul>");
  write(out, "</body></html>");
}

void JsonEventList::addEvent(cEvent* event, bool scan_images = false)
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

  serEvent.Images = NULL;
  serEvent.ImagesCount = 0;

  if ( scan_images ) {
     std::string wildcardpath = (std::string)"/var/cache/vdr/epgimages/" + itostr(serEvent.Id) + (std::string)"*.*";
     std::vector< std::string > images;
     int found = scanForFiles(wildcardpath, images);
     if (found > 0) {
        serEvent.Images = new cxxtools::String[found];
        serEvent.ImagesCount = found;
        for (int i=0;i<found;i++) {
           serEvent.Images[i] = UTF8Decode(images[i]);
        }
     }
  }

  serEvents.push_back(serEvent);
}

void JsonEventList::finish()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serEvents, "events");
  serializer.finish();
}

void XmlEventList::init()
{
  write(out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
  write(out, "<events xmlns=\"http://www.domain.org/restfulapi/2011/events-xml\">\n");
}

void XmlEventList::addEvent(cEvent* event, bool scan_images = false)
{
  std::string eventTitle;
  std::string eventShortText;
  std::string eventDescription;

  if ( event->Title() == NULL ) { eventTitle = ""; } else { eventTitle = event->Title(); }
  if ( event->ShortText() == NULL ) { eventShortText = ""; } else { eventShortText = event->ShortText(); }
  if ( event->Description() == NULL ) { eventDescription = ""; } else { eventDescription = event->Description(); }

  write(out, " <event>\n");
  write(out, (const char*)cString::sprintf("  <param name=\"id\">%i</param>\n", event->EventID()));
  write(out, (const char*)cString::sprintf("  <param name=\"title\">%s</param>\n", encodeToXml(eventTitle).c_str()));
  write(out, (const char*)cString::sprintf("  <param name=\"short_text\">%s</param>\n", encodeToXml(eventShortText).c_str()));
  write(out, (const char*)cString::sprintf("  <param name=\"description\">%s</param>\n", encodeToXml(eventDescription).c_str()));
  write(out, (const char*)cString::sprintf("  <param name=\"start_time\">%i</param>\n", (int)event->StartTime()));
  write(out, (const char*)cString::sprintf("  <param name=\"duration\">%i</param>\n", event->Duration()));

  if ( scan_images ) {
     std::string wildcardpath = (std::string)"/var/cache/vdr/epgimages/" + itostr(event->EventID()) + (std::string)"*.*";
     std::vector< std::string > images;
     int found = scanForFiles(wildcardpath, images);
     write(out, "  <param name=\"images\">");
     for (int i=0;i<found;i++) {
        write(out, (const char*)cString::sprintf("   <image>%s</image>", encodeToXml(images[i]).c_str()));
     }
     write(out, "  </param>");
  }

  write(out, " </event>\n");
}

void XmlEventList::finish()
{
  write(out, "</events>");
}
