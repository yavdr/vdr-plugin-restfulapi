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
  QueryHandler q("/events", request);

  EventList* eventList;

  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     eventList = (EventList*)new JsonEventList(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     eventList = (EventList*)new HtmlEventList(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     eventList = (EventList*)new XmlEventList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  std::string channel_id = q.getParamAsString(0);
  int timespan = q.getParamAsInt(1);
  int from = q.getParamAsInt(2);

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");

  bool scan_images = q.getOptionAsString("images") == "true" ? true : false;
  
  cChannel* channel = VdrExtension::getChannel(channel_id);
  if ( channel == NULL ) { 
     reply.httpReturn(404, "Channel with number _xx_ not found!"); 
     return; 
  }

  if ( from <= -1 ) from = time(NULL); // default time is now
  if ( timespan <= -1 ) timespan = 3600; // default timespan is one hour
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
  
  if ( start_filter >= 0 && limit_filter >= 1 ) {
     eventList->activateLimit(start_filter, limit_filter);
  }

  eventList->init();
  int old = 0;
  for(cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event))
  {
    int ts = event->StartTime();
    int te = ts + event->Duration();
    if ( (ts <= to && te > from) || (te > from && timespan == 0) ) {
       eventList->addEvent(event, scan_images);
    }else{
      if(ts > to) break;
      if(te <= from) {
        old++;
      }
    }
  }
  eventList->setTotal( Schedule->Events()->Count() - old );
  eventList->finish();
  delete eventList;
}

void EventsResponder::replyImage(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  StreamExtension se(&out);
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
  if ( !se.writeBinary(absolute_path) ) {
     reply.httpReturn(404, "Could not find image!");
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
  if (e.Images != NULL) {
      std::vector< cxxtools::String > imgs;
     for(int i=0;i<e.ImagesCount;i++) {
        imgs.push_back(e.Images[i]);
     }
     si.addMember("images") <<= imgs;
     delete[] e.Images;
  }
}

EventList::EventList(std::ostream *_out) {
  s = new StreamExtension(_out);
}

EventList::~EventList()
{
  delete s;
}

void HtmlEventList::init()
{
  s->writeHtmlHeader();
  s->write("<ul>");
}

void HtmlEventList::addEvent(cEvent* event, bool scan_images = false)
{
  if ( filtered() ) return;
  s->write("<li>");
  s->write((char*)event->Title()); //TODO: add more infos
  s->write("\n");
}

void HtmlEventList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonEventList::addEvent(cEvent* event, bool scan_images = false)
{
  if ( filtered() ) return;
  

  cxxtools::String eventTitle;
  cxxtools::String eventShortText;
  cxxtools::String eventDescription;
  cxxtools::String empty = StringExtension::UTF8Decode("");
  SerEvent serEvent;

  if( !event->Title() ) { eventTitle = empty; } else { eventTitle = StringExtension::UTF8Decode(event->Title()); }
  if( !event->ShortText() ) { eventShortText = empty; } else { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
  if( !event->Description() ) { eventDescription = empty; } else { eventDescription = StringExtension::UTF8Decode(event->Description()); }

  serEvent.Id = event->EventID();
  serEvent.Title = eventTitle;
  serEvent.ShortText = eventShortText;
  serEvent.Description = eventDescription;
  serEvent.StartTime = event->StartTime();
  serEvent.Duration = event->Duration();

  serEvent.Images = NULL;
  serEvent.ImagesCount = 0;

  if ( scan_images ) {
     cxxtools::Regex regex( StringExtension::itostr(serEvent.Id) + (std::string)"(_[0-9]+)?\.[a-z]{3,4}");
     std::string wildcardpath = (std::string)"/var/cache/vdr/epgimages/" + StringExtension::itostr(serEvent.Id) + (std::string)"*";
     std::vector< std::string > images;
     int found = VdrExtension::scanForFiles(wildcardpath, images, regex);
     if (found > 0) {
        serEvent.Images = new cxxtools::String[found];
        serEvent.ImagesCount = found;
        for (int i=0;i<found;i++) {
           serEvent.Images[i] = StringExtension::UTF8Decode(images[i]);
        }
     }
  }

  serEvents.push_back(serEvent);
}

void JsonEventList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(serEvents, "events");
  serializer.serialize(serEvents.size(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlEventList::init()
{
  s->writeXmlHeader();  
  s->write("<events xmlns=\"http://www.domain.org/restfulapi/2011/events-xml\">\n");
}

void XmlEventList::addEvent(cEvent* event, bool scan_images = false)
{
  if ( filtered() ) return;

  std::string eventTitle;
  std::string eventShortText;
  std::string eventDescription;

  if ( event->Title() == NULL ) { eventTitle = ""; } else { eventTitle = event->Title(); }
  if ( event->ShortText() == NULL ) { eventShortText = ""; } else { eventShortText = event->ShortText(); }
  if ( event->Description() == NULL ) { eventDescription = ""; } else { eventDescription = event->Description(); }

  s->write(" <event>\n");
  s->write((const char*)cString::sprintf("  <param name=\"id\">%i</param>\n", event->EventID()));
  s->write((const char*)cString::sprintf("  <param name=\"title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str()));
  s->write((const char*)cString::sprintf("  <param name=\"start_time\">%i</param>\n", (int)event->StartTime()));
  s->write((const char*)cString::sprintf("  <param name=\"duration\">%i</param>\n", event->Duration()));

  if ( scan_images ) {
     cxxtools::Regex regex( StringExtension::itostr(event->EventID()) + (std::string)"(_[0-9]+)?\.[a-z]{3,4}");
     std::string wildcardpath = (std::string)"/var/cache/vdr/epgimages/" + StringExtension::itostr(event->EventID()) + (std::string)"*";
     std::vector< std::string > images;
     int found = VdrExtension::scanForFiles(wildcardpath, images, regex);
     s->write("  <param name=\"images\">\n");
     for (int i=0;i<found;i++) {
        s->write((const char*)cString::sprintf("   <image>%s</image>\n", StringExtension::encodeToXml(images[i]).c_str()));
     }
     s->write("  </param>\n");
  }

  s->write(" </event>\n");
}

void XmlEventList::finish()
{
  s->write((const char*)cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</events>");
}
