#include "events.h"

void EventsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( (int)request.url().find("/events/image/") == 0 ) {
     replyImage(out, request, reply);
  } else if ( (int)request.url().find("/events/search") == 0 ){
     replySearchResult(out, request, reply);
  } else {
     replyEvents(out, request, reply);
  }
}

void EventsResponder::replyEvents(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/events", request, reply);

  if ( request.method() != "GET") {

     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }


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

  cChannel* channel = VdrExtension::getChannel(channel_id);
  if ( channel == NULL ) { 
     std::string error_message = (std::string)"Could not find channel with id: " + channel_id + (std::string)"!";
     reply.httpReturn(404, error_message); 
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
       eventList->addEvent(event);
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
  QueryHandler q("/events/image", request, reply);
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  StreamExtension se(&out);
  int eventid = q.getParamAsInt(0);
  int number = q.getParamAsInt(1);
  
  std::vector< std::string > images;
  
  FileCaches::get()->searchEventImages(eventid, images);

  if (number < 0 || number >= (int)images.size()) {
     reply.httpReturn(404, "Could not find image because of invalid image number!");
     return;
  }

  std::string image = images[number];
  std::string type = image.substr(image.find_last_of(".")+1);
  std::string contenttype = (std::string)"image/" + type;
  std::string path = Settings::get()->EpgImageDirectory() + (std::string)"/" + image;
 
  if ( se.writeBinary(path) ) {
     reply.addHeader("Content-Type", contenttype.c_str());
  } else {
     reply.httpReturn(404, "Could not find image!");
  }
}

void EventsResponder::replySearchResult(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/events/search", request, reply);

  if ( request.method() != "POST") {
     reply.httpReturn(403, "To search for information use the POST method!");
     return;
  }

  StreamExtension se(&out);

  std::string query = q.getBodyAsString("query");
  esyslog("restfulapi, query:/%s/", query.c_str());
  int mode = q.getBodyAsInt("mode");// search mode (0=phrase, 1=and, 2=or, 3=regular expression)
  std::string channelid = q.getBodyAsString("channel"); //id !!
  bool use_title = q.getBodyAsBool("use_title");
  bool use_subtitle = q.getBodyAsBool("use_subtitle");
  bool use_description = q.getBodyAsBool("use_description");

  if ( query.length() == 0 ) {
     reply.httpReturn(402, "Query required");
     return;
  }

  int channel = 0;
  cChannel* channelInstance = VdrExtension::getChannel(channelid);
  if (channelInstance != NULL) {
     channel = channelInstance->Number();
  }

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
  eventList->init();
  
  if (!use_title && !use_subtitle && !use_description)
     use_title = true;
  if (mode < 0 || mode > 3) 
     mode = 0;
  if (channel < 0 || channel > Channels.Count())
     channel = 0;
  if (query.length() > 100)
     query = query.substr(0,100); //don't allow more than 100 characters, NOTE: maybe I should add a limitation to the Responderclass?

  struct Epgsearch_searchresults_v1_0* epgquery = new struct Epgsearch_searchresults_v1_0;
  epgquery->query = (char*)query.c_str();
  epgquery->mode = mode;
  epgquery->channelNr = channel;
  epgquery->useTitle = use_title;
  epgquery->useSubTitle = use_subtitle;
  epgquery->useDescription = use_description;
 
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  if ( start_filter >= 0 && limit_filter >= 1 ) {
     eventList->activateLimit(start_filter, limit_filter);
  }

  int total = 0; 

  cPlugin *Plugin = cPluginManager::GetPlugin("epgsearch");
  if (Plugin) {
     if (Plugin->Service("Epgsearch-searchresults-v1.0", NULL)) {
        if (Plugin->Service("Epgsearch-searchresults-v1.0", epgquery)) {
           cList< Epgsearch_searchresults_v1_0::cServiceSearchResult>* result = epgquery->pResultList;
           Epgsearch_searchresults_v1_0::cServiceSearchResult* item = NULL;
           if (result != NULL) {
              for(int i=0;i<result->Count();i++) {
                 item = result->Get(i);
                 eventList->addEvent(((cEvent*)item->event));
                 total++;
              }
           }
        } else {
           reply.httpReturn(406, "Internal (epgsearch) error, check parameters.");
        }
     } else {
        reply.httpReturn(405, "Plugin-service not available.");
     }
  } else {
     reply.httpReturn(404, "Plugin not installed!");
  }
  eventList->setTotal(total);
  eventList->finish();
  delete eventList;
  delete epgquery;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e)
{
  si.addMember("id") <<= e.Id;
  si.addMember("title") <<= e.Title;
  si.addMember("short_text") <<= e.ShortText;
  si.addMember("description") <<= e.Description;
  si.addMember("start_time") <<= e.StartTime;
  si.addMember("duration") <<= e.Duration;
  si.addMember("images") <<= e.Images;
}

EventList::EventList(std::ostream *_out) {
  s = new StreamExtension(_out);
  total = 0;
}

EventList::~EventList()
{
  delete s;
}

void HtmlEventList::init()
{
  s->writeHtmlHeader( "HtmlEventList" );
  s->write("<ul>");
}

void HtmlEventList::addEvent(cEvent* event)
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

void JsonEventList::addEvent(cEvent* event)
{
  if ( filtered() ) return;

  cxxtools::String eventTitle;
  cxxtools::String eventShortText;
  cxxtools::String eventDescription;
  cxxtools::String empty = StringExtension::UTF8Decode("");
  cxxtools::String channelStr = StringExtension::UTF8Decode((const char*)event->ChannelID().ToString());

  SerEvent serEvent;

  if( !event->Title() ) { eventTitle = empty; } else { eventTitle = StringExtension::UTF8Decode(event->Title()); }
  if( !event->ShortText() ) { eventShortText = empty; } else { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
  if( !event->Description() ) { eventDescription = empty; } else { eventDescription = StringExtension::UTF8Decode(event->Description()); }

  serEvent.Id = event->EventID();
  serEvent.Title = eventTitle;
  serEvent.ShortText = eventShortText;
  serEvent.Description = eventDescription;
  serEvent.Channel = channelStr;
  serEvent.StartTime = event->StartTime();
  serEvent.Duration = event->Duration();

  std::vector< std::string > images;
  FileCaches::get()->searchEventImages((int)event->EventID(), images);
  serEvent.Images = images.size();

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

void XmlEventList::addEvent(cEvent* event)
{
  if ( filtered() ) return;

  std::string eventTitle;
  std::string eventShortText;
  std::string eventDescription;

  if ( event->Title() == NULL ) { eventTitle = ""; } else { eventTitle = event->Title(); }
  if ( event->ShortText() == NULL ) { eventShortText = ""; } else { eventShortText = event->ShortText(); }
  if ( event->Description() == NULL ) { eventDescription = ""; } else { eventDescription = event->Description(); }

  s->write(" <event>\n");
  s->write(cString::sprintf("  <param name=\"id\">%i</param>\n", event->EventID()));
  s->write(cString::sprintf("  <param name=\"title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str()));
  s->write(cString::sprintf("  <param name=\"short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str()));
  s->write(cString::sprintf("  <param name=\"description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str()));

  s->write(cString::sprintf("  <param name=\"channel\">%s</param>\n", StringExtension::encodeToXml((const char*)event->ChannelID().ToString()).c_str()));

  s->write(cString::sprintf("  <param name=\"start_time\">%i</param>\n", (int)event->StartTime()));
  s->write(cString::sprintf("  <param name=\"duration\">%i</param>\n", event->Duration()));

  std::vector< std::string > images;
  FileCaches::get()->searchEventImages((int)event->EventID(), images);
  s->write(cString::sprintf("  <param name=\"images\">%i</param>\n", (int)images.size()));

  s->write(" </event>\n");
}

void XmlEventList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</events>");
}
