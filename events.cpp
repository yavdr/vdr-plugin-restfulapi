#include "events.h"
using namespace std;

void EventsResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  if ( (int)request.url().find("/events/image/") == 0 ) {
     replyImage(out, request, reply);
  } else if ( (int)request.url().find("/events/search") == 0 ) {
     replySearchResult(out, request, reply);
  } else {
     replyEvents(out, request, reply);
  }
}

void EventsResponder::replyEvents(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/events", request);

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

  string channel_id = q.getParamAsString(0);
  int timespan = q.getOptionAsInt("timespan");//q.getParamAsInt(1);
  int from = q.getOptionAsInt("from");//q.getParamAsInt(2);

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  
  int event_id = q.getParamAsInt(1);//q.getOptionAsInt("eventid");

  string onlyCount = q.getOptionAsString("only_count");

  cChannel* channel = VdrExtension::getChannel(channel_id);
  if ( channel == NULL ) { 
     /*reply.addHeader("Content-Type", "application/octet-stream");
     string error_message = (string)"Could not find channel with id: " + channel_id + (string)"!";
     reply.httpReturn(404, error_message); 
     return;*/
  }

  int channel_limit = q.getOptionAsInt("chevents");
  if ( channel_limit <= -1 ) channel_limit = 0; // default channel events is 0 -> all
  
  int channel_from = q.getOptionAsInt("chfrom");
  if ( channel_from <= -1 || channel != NULL ) channel_from = 0; // default channel number is 0
  
  int channel_to = q.getOptionAsInt("chto");
  if ( channel_to <= 0 || channel != NULL ) channel_to = Channels.Count();
 
  if ( from <= -1 ) from = time(NULL); // default time is now
  if ( timespan <= -1 ) timespan = 0; // default timespan is 0, which means all entries will be returned
  int to = from + timespan;

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);

  if( !Schedules ) {
     reply.httpReturn(404, "Could not find schedules!");
     return;
  }

  if ( start_filter >= 0 && limit_filter >= 1 ) {
     eventList->activateLimit(start_filter, limit_filter);
  }

  bool initialized = false;
  int total = 0;
  for(int i=0; i<Channels.Count(); i++) {
     if (Channels.Get(i)->GroupSep()) {  // we have a group-separator
        if (channel_from > 0) channel_from += 1;
        if (channel_to > 0 && channel_to < Channels.Count()) channel_to += 1;
        continue;
     }

     const cSchedule *Schedule = Schedules->GetSchedule(Channels.Get(i)->GetChannelID());

     if (!Schedule) { // we have a channel without an epg
        channel_from += 1;
        if (channel_to < Channels.Count()) channel_to += 1;
     }

     if ((channel == NULL || strcmp(channel->GetChannelID().ToString(), Channels.Get(i)->GetChannelID().ToString()) == 0) && (i >= channel_from && i <= channel_to)) {
        if (!Schedule) {
           if (channel != NULL) {
              reply.httpReturn(404, "Could not find schedule!");
              return;
           }
        } else {
           if (!initialized) {
              eventList->init();
              initialized = true;
           }

           int old = 0;
           int channel_events = 0;
           for(cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event)) {
              int ts = event->StartTime();
              int te = ts + event->Duration();
              if ((ts <= to && te > from) || (te > from && timespan == 0)) {
                 if (channel_limit == 0 || channel_limit > channel_events) {
                    if ((event_id < 0 || event_id == (int)event->EventID()) && onlyCount != "true") {
                       eventList->addEvent(event);
                       channel_events++;
                    }
                 }
              } else {
                 if (ts > to) break;
                 if (te <= from) old++;
              }
           }
           total += (Schedule->Events()->Count() - old);
        }
     }
  }
  eventList->setTotal(total);
  eventList->finish();
  delete eventList;
}

void EventsResponder::replyImage(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/events/image", request);
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  StreamExtension se(&out);
  int eventid = q.getParamAsInt(0);
  int number = q.getParamAsInt(1);
  
  vector< string > images;
  
  FileCaches::get()->searchEventImages(eventid, images);

  if (number < 0 || number >= (int)images.size()) {
     reply.httpReturn(404, "Could not find image because of invalid image number!");
     return;
  }

  string image = images[number];
  string type = image.substr(image.find_last_of(".")+1);
  string contenttype = (string)"image/" + type;
  string path = Settings::get()->EpgImageDirectory() + (string)"/" + image;
 
  if ( se.writeBinary(path) ) {
     reply.addHeader("Content-Type", contenttype.c_str());
  } else {
     reply.httpReturn(404, "Could not find image!");
  }
}

void EventsResponder::replySearchResult(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/events/search", request);

  if ( request.method() != "POST") {
     reply.httpReturn(403, "To search for information use the POST method!");
     return;
  }

  StreamExtension se(&out);

  string query = q.getBodyAsString("query");
 
  int mode = q.getBodyAsInt("mode");// search mode (0=phrase, 1=and, 2=or, 3=regular expression)
  string channelid = q.getBodyAsString("channel"); //id !!
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
  si.addMember("channel") <<= e.Channel;
  si.addMember("channel_name") <<= e.ChannelName;
  si.addMember("duration") <<= e.Duration;
  si.addMember("table_id") <<= e.TableID;
  si.addMember("version") <<= e.Version;
  si.addMember("images") <<= e.Images;
  si.addMember("timer_exists") <<= e.TimerExists;
  si.addMember("timer_active") <<= e.TimerActive;
  si.addMember("timer_id") <<= e.TimerId;
#if APIVERSNUM > 10710 || EPGHANDLER
  si.addMember("parental_rating") <<= e.ParentalRating;
#endif
  si.addMember("vps") <<= e.Vps;

  vector< SerComponent > components;
  if ( e.Instance->Components() != NULL ) {
     for(int i=0;i<e.Instance->Components()->NumComponents();i++) {
        tComponent* comp = e.Instance->Components()->Component(i);
        SerComponent component;
        component.Stream = (int)comp->stream;
        component.Type = (int)comp->type;
        component.Language = StringExtension::UTF8Decode("");
        if(comp->language != NULL) component.Language = StringExtension::UTF8Decode(string(comp->language));
        component.Description = StringExtension::UTF8Decode("");
        if(comp->description != NULL) component.Description = StringExtension::UTF8Decode(string(comp->description));
        components.push_back(component); 
     }
  }

  si.addMember("components") <<= components;

#if APIVERSNUM > 10710 || EPGHANDLER
  vector< cxxtools::String > contents;
  int counter = 0;
  uchar content = e.Instance->Contents(counter);
  while (content != 0) {
     contents.push_back(StringExtension::UTF8Decode(cEvent::ContentToString(content)));
     counter++;
     content = e.Instance->Contents(counter);
  }
  si.addMember("contents") <<= contents;

  vector< int > raw_contents;
  counter = 0;
  uchar raw_content = e.Instance->Contents(counter);
  while (raw_content != 0) {
     raw_contents.push_back(raw_content);
     counter++;
     raw_content = e.Instance->Contents(counter);
  }
  si.addMember("raw_contents") <<= raw_contents;
#endif

#ifdef EPG_DETAILS_PATCH
  si.addMember("details") <<= *e.Details;
#endif
  si.addMember("additional_media") <<= e.Scraper;
  si.addMember("poster") <<= e.ScraperPoster;
  //  si.addMember("fanart") <<= e.ScraperFanart;
  si.addMember("banner") <<= e.ScraperBanner;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerComponent& c)
{
  si.addMember("stream") <<= c.Stream;
  si.addMember("type") <<= c.Type;
  si.addMember("language") <<= c.Language;
  si.addMember("description") <<= c.Description;
}

#ifdef EPG_DETAILS_PATCH
void operator<<= (cxxtools::SerializationInfo& si, const struct tEpgDetail& e)
{
  si.addMember("key") <<= StringExtension::UTF8Decode(e.key);
  si.addMember("value") <<= StringExtension::UTF8Decode(e.value);
}
#endif

EventList::EventList(ostream *_out) {
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
  cxxtools::String channelName = StringExtension::UTF8Decode((const char*)Channels.GetByChannelID(event->ChannelID(), true)->Name());

  SerEvent serEvent;

  if( !event->Title() ) { eventTitle = empty; } else { eventTitle = StringExtension::UTF8Decode(event->Title()); }
  if( !event->ShortText() ) { eventShortText = empty; } else { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
  if( !event->Description() ) { eventDescription = empty; } else { eventDescription = StringExtension::UTF8Decode(event->Description()); }
    
  cMovie movie;
  cSeries series;
  ScraperGetEventType call;
  bool hasAdditionalMedia = false;
  bool isMovie = false;
  bool isSeries = false;
   
  static cPlugin *pScraper = GetScraperPlugin();
  if (pScraper) {
     ScraperGetEventType call;
     call.event = event;
     int seriesId = 0;
     int episodeId = 0;
     int movieId = 0;
     if (pScraper->Service("GetEventType", &call)) {
        //esyslog("restfulapi: Type detected: %d, seriesId %d, episodeId %d, movieId %d", call.type, call.seriesId, call.episodeId, call.movieId);
        seriesId = call.seriesId;
        episodeId = call.episodeId;
        movieId = call.movieId;
     }
     if (seriesId > 0) {
        series.seriesId = seriesId;
        series.episodeId = episodeId;
        if (pScraper->Service("GetSeries", &series)) {
           hasAdditionalMedia = true;
           isSeries = true;
        }
     } else if (movieId > 0) {
        movie.movieId = movieId;
        if (pScraper->Service("GetMovie", &movie)) {
           hasAdditionalMedia = true;
           isMovie = true;
        }
     }
  }

  serEvent.Id = event->EventID();
  serEvent.Title = eventTitle;
  serEvent.ShortText = eventShortText;
  serEvent.Description = eventDescription;
  serEvent.Channel = channelStr;
  serEvent.ChannelName = channelName;
  serEvent.StartTime = event->StartTime();
  serEvent.Duration = event->Duration();
  serEvent.TableID = (int)event->TableID();
  serEvent.Version = (int)event->Version();
#if APIVERSNUM > 10710 || EPGHANDLER
  serEvent.ParentalRating = event->ParentalRating();
#endif
  serEvent.Vps = event->Vps();
  serEvent.Instance = event;

  cTimer* timer = VdrExtension::TimerExists(event);
  serEvent.TimerExists = timer != NULL ? true : false;
  serEvent.TimerActive = false;
  if ( timer != NULL ) {
     serEvent.TimerActive = timer->Flags() & 0x01 == 0x01 ? true : false;
     serEvent.TimerId = StringExtension::UTF8Decode(VdrExtension::getTimerID(timer));
  }

  vector< string > images;
  FileCaches::get()->searchEventImages((int)event->EventID(), images);
  serEvent.Images = images.size();

#ifdef EPG_DETAILS_PATCH
  serEvent.Details = (vector<tEpgDetail>*)&event->Details();
#endif
    
  if (hasAdditionalMedia) {
     if (isSeries) {
        serEvent.Scraper = StringExtension::UTF8Decode("series");
        if (series.posters.size() > 0) { /*
           int posters = series.posters.size();
           for (int i = 0; i < posters;i++) {
               serEvent.TvScraperPoster = StringExtension::UTF8Decode(series.posters[i].path);
            } */
            serEvent.ScraperPoster = StringExtension::UTF8Decode(series.posters[0].path);
        }
        if (series.banners.size() > 0) {
           serEvent.ScraperBanner = StringExtension::UTF8Decode(series.banners[0].path);
        }
     } else if (isMovie) {
        serEvent.Scraper = StringExtension::UTF8Decode("movie");
        if ((movie.poster.width > 0) && (movie.poster.height > 0) && (movie.poster.path.size() > 0)) {
           serEvent.ScraperPoster = StringExtension::UTF8Decode(movie.poster.path);
        }
     }
  }

  serEvents.push_back(serEvent);
}

void JsonEventList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.beautify();
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

  string eventTitle;
  string eventShortText;
  string eventDescription;

  if ( event->Title() == NULL ) { eventTitle = ""; } else { eventTitle = event->Title(); }
  if ( event->ShortText() == NULL ) { eventShortText = ""; } else { eventShortText = event->ShortText(); }
  if ( event->Description() == NULL ) { eventDescription = ""; } else { eventDescription = event->Description(); }
    
  cMovie movie;
  cSeries series;
  ScraperGetEventType call;
  bool hasAdditionalMedia = false;
  bool isMovie = false;
  bool isSeries = false;

  static cPlugin *pScraper = GetScraperPlugin();
  if (pScraper) {
     ScraperGetEventType call;
     call.event = event;
     int seriesId = 0;
     int episodeId = 0;
     int movieId = 0;
     if (pScraper->Service("GetEventType", &call)) {
        //esyslog("restfulapi: Type detected: %d, seriesId %d, episodeId %d, movieId %d", call.type, call.seriesId, call.episodeId, call.movieId);
        seriesId = call.seriesId;
        episodeId = call.episodeId;
        movieId = call.movieId;
     }
     if (seriesId > 0) {
        series.seriesId = seriesId;
        series.episodeId = episodeId;
        if (pScraper->Service("GetSeries", &series)) {
           hasAdditionalMedia = true;
           isSeries = true;
        }
     } else if (movieId > 0) {
        movie.movieId = movieId;
        if (pScraper->Service("GetMovie", &movie)) {
           hasAdditionalMedia = true;
           isMovie = true;
        }
     }
  }
   
  s->write(" <event>\n");
  s->write(cString::sprintf("  <param name=\"id\">%i</param>\n", event->EventID()));
  s->write(cString::sprintf("  <param name=\"title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str()));
  s->write(cString::sprintf("  <param name=\"short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str()));
  s->write(cString::sprintf("  <param name=\"description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str()));

  s->write(cString::sprintf("  <param name=\"channel\">%s</param>\n", StringExtension::encodeToXml((const char*)event->ChannelID().ToString()).c_str()));
  s->write(cString::sprintf("  <param name=\"channel_name\">%s</param>\n", StringExtension::encodeToXml((const char*)Channels.GetByChannelID(event->ChannelID(), true)->Name()).c_str()));

  s->write(cString::sprintf("  <param name=\"start_time\">%i</param>\n", (int)event->StartTime()));
  s->write(cString::sprintf("  <param name=\"duration\">%i</param>\n", event->Duration()));
  s->write(cString::sprintf("  <param name=\"table_id\">%i</param>\n", (int)event->TableID()));
  s->write(cString::sprintf("  <param name=\"version\">%i</param>\n", (int)event->Version()));
#if APIVERSNUM > 10710 || EPGHANDLER
  s->write(cString::sprintf("  <param name=\"parental_rating\">%i</param>\n", event->ParentalRating()));
#endif
  s->write(cString::sprintf("  <param name=\"vps\">%i</param>\n", (int)event->Vps()));
  
#ifdef EPG_DETAILS_PATCH
  s->write("  <param name=\"details\">\n");
  for(int i=0;i<(int)event->Details().size();i++) {
     string key = event->Details()[i].key;
     string value = event->Details()[i].value;
     s->write(cString::sprintf("   <detail key=\"%s\">%s</detail>\n", StringExtension::encodeToXml(key.c_str()).c_str(), StringExtension::encodeToXml(value.c_str()).c_str()));
  }
  s->write("  </param>\n");
#endif

  vector< string > images;
  FileCaches::get()->searchEventImages((int)event->EventID(), images);
  s->write(cString::sprintf("  <param name=\"images\">%i</param>\n", (int)images.size()));

  cTimer* timer = VdrExtension::TimerExists(event);
  bool timer_exists = timer != NULL ? true : false;
  bool timer_active = false;
  string timer_id = "";
  if ( timer_exists ) {
     timer_active = timer->Flags() & 0x01 == 0x01 ? true : false;
     timer_id = VdrExtension::getTimerID(timer);
  }

  s->write("  <param name=\"components\">\n");
  if (event->Components() != NULL) {
     cComponents* components = (cComponents*)event->Components();
     for(int i=0;i<components->NumComponents();i++) {
        tComponent* component = components->Component(i);

        string language = ""; 
        if (component->language != NULL) language = string(component->language);

        string description = "";
        if (component->description != NULL) description = string(component->description);

        s->write(cString::sprintf("   <component stream=\"%i\" type=\"%i\" language=\"%s\" description=\"%s\" />\n", 
                                  (int)component->stream, (int)component->type, language.c_str(), description.c_str()));
     }
  }
  s->write("  </param>\n");

#if APIVERSNUM > 10710 || EPGHANDLER
  s->write("  <param name=\"contents\">\n");
  int counter = 0;
  uchar content = event->Contents(counter);
  while(content != 0) {
    counter++;
    s->write(cString::sprintf("   <content name=\"%s\" />\n", StringExtension::encodeToXml(cEvent::ContentToString(content)).c_str() ));
    content = event->Contents(counter);
  }
  s->write("  </param>\n");

  s->write("  <param name=\"raw_contents\">\n");
  counter = 0;
  content = event->Contents(counter);
  while(content != 0) {
    counter++;
    s->write(cString::sprintf("   <raw_content name=\"%i\" />\n", content));
    content = event->Contents(counter);
  }
  s->write("  </param>\n");
#endif

  s->write(cString::sprintf("  <param name=\"timer_exists\">%s</param>\n", (timer_exists ? "true" : "false")));
  s->write(cString::sprintf("  <param name=\"timer_active\">%s</param>\n", (timer_active ? "true" : "false")));
  s->write(cString::sprintf("  <param name=\"timer_id\">%s</param>\n", timer_id.c_str()));
    
  if (hasAdditionalMedia) {
     if (isSeries) {
        s->write("  <param name=\"additional_media\" type=\"series\">\n");
        s->write(cString::sprintf("    <series_id>%i</series_id>\n", series.seriesId));
        if (series.episodeId > 0) {
           s->write(cString::sprintf("    <episode_id>%i</episode_id>\n", series.episodeId));
        }
        if (series.name != "") {
           s->write(cString::sprintf("    <name>%s</name>\n", StringExtension::encodeToXml(series.name).c_str()));
        }
        if (series.overview != "") {
           s->write(cString::sprintf("    <overview>%s</overview>\n", StringExtension::encodeToXml(series.overview).c_str()));
        }
        if (series.firstAired != "") {
           s->write(cString::sprintf("    <first_aired>%s</first_aired>\n", StringExtension::encodeToXml(series.firstAired).c_str()));
        }
        if (series.network != "") {
           s->write(cString::sprintf("    <network>%s</network>\n", StringExtension::encodeToXml(series.network).c_str()));
        }
        if (series.genre != "") {
           s->write(cString::sprintf("    <genre>%s</genre>\n", StringExtension::encodeToXml(series.genre).c_str()));
        }
        if (series.rating > 0) {
           s->write(cString::sprintf("    <rating>%.2f</rating>\n", series.rating));
        }
        if (series.status != "") {
           s->write(cString::sprintf("    <status>%s</status>\n", StringExtension::encodeToXml(series.status).c_str()));
        }
         
        if (series.episode.number > 0) {
           s->write(cString::sprintf("    <episode_number>%i</episode_number>\n", series.episode.number));
           s->write(cString::sprintf("    <episode_season>%i</episode_season>\n", series.episode.season));
           s->write(cString::sprintf("    <episode_name>%s</episode_name>\n", StringExtension::encodeToXml(series.episode.name).c_str()));
           s->write(cString::sprintf("    <episode_first_aired>%s</episode_first_aired>\n", StringExtension::encodeToXml(series.episode.firstAired).c_str()));
           s->write(cString::sprintf("    <episode_guest_stars>%s</episode_guest_stars>\n", StringExtension::encodeToXml(series.episode.guestStars).c_str()));
           s->write(cString::sprintf("    <episode_overview>%s</episode_overview>\n", StringExtension::encodeToXml(series.episode.overview).c_str()));
           s->write(cString::sprintf("    <episode_rating>%.2f</episode_rating>\n", series.episode.rating));
           s->write(cString::sprintf("    <episode_image>%s</episode_image>\n", StringExtension::encodeToXml(series.episode.episodeImage.path).c_str()));
        }
         
        if (series.actors.size() > 0) {
           int _actors = series.actors.size();
           for (int i = 0; i < _actors; i++) {
               s->write(cString::sprintf("    <actor name=\"%s\" role=\"%s\" thumb=\"%s\"/>\n",
                                         StringExtension::encodeToXml(series.actors[i].name).c_str(),
                                         StringExtension::encodeToXml(series.actors[i].role).c_str(),
                                         StringExtension::encodeToXml(series.actors[i].actorThumb.path).c_str() ));
           }
        }
        if (series.posters.size() > 0) {
           int _posters = series.posters.size();
           for (int i = 0; i < _posters; i++) {
               if ((series.posters[i].width > 0) && (series.posters[i].height > 0))
                  s->write(cString::sprintf("    <poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                            StringExtension::encodeToXml(series.posters[i].path).c_str(), series.posters[i].width, series.posters[i].height));
           }
        }
        if (series.banners.size() > 0) {
           int _banners = series.banners.size();
           for (int i = 0; i < _banners; i++) {
               if ((series.banners[i].width > 0) && (series.banners[i].height > 0))
                  s->write(cString::sprintf("    <banner path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                            StringExtension::encodeToXml(series.banners[i].path).c_str(), series.banners[i].width, series.banners[i].height));
           }
        }
        if (series.fanarts.size() > 0) {
           int _fanarts = series.fanarts.size();
           for (int i = 0; i < _fanarts; i++) {
               if ((series.fanarts[i].width > 0) && (series.fanarts[i].height > 0))
                  s->write(cString::sprintf("    <fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                            StringExtension::encodeToXml(series.fanarts[i].path).c_str(), series.fanarts[i].width, series.fanarts[i].height));
           }
        }
        if ((series.seasonPoster.width > 0) && (series.seasonPoster.height > 0) && (series.seasonPoster.path.size() > 0)) {
           s->write(cString::sprintf("    <season_poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(series.seasonPoster.path).c_str(), series.seasonPoster.width, series.seasonPoster.height));
        }
        s->write("  </param>\n");
         
     } else if (isMovie) {
        s->write("  <param name=\"additional_media\" type=\"movie\">\n");
        s->write(cString::sprintf("    <movie_id>%i</movie_id>\n", movie.movieId));
        if (movie.title != "") {
           s->write(cString::sprintf("    <title>%s</title>\n", StringExtension::encodeToXml(movie.title).c_str()));
        }
        if (movie.originalTitle != "") {
           s->write(cString::sprintf("    <original_title>%s</original_title>\n", StringExtension::encodeToXml(movie.originalTitle).c_str()));
        }
        if (movie.tagline != "") {
           s->write(cString::sprintf("    <tagline>%s</tagline>\n", StringExtension::encodeToXml(movie.tagline).c_str()));
        }
        if (movie.overview != "") {
           s->write(cString::sprintf("    <overview>%s</overview>\n", StringExtension::encodeToXml(movie.overview).c_str()));
        }
        if (movie.adult) {
           s->write("    <adult>true</adult>\n");
        }
        if (movie.collectionName != "") {
           s->write(cString::sprintf("    <collection_name>%s</collection_name>\n", StringExtension::encodeToXml(movie.collectionName).c_str()));
        }
        if (movie.budget > 0) {
           s->write(cString::sprintf("    <budget>%i</budget>\n", movie.budget));
        }
        if (movie.revenue > 0) {
           s->write(cString::sprintf("    <revenue>%i</revenue>\n", movie.revenue));
        }
        if (movie.genres != "") {
           s->write(cString::sprintf("    <genres>%s</genres>\n", StringExtension::encodeToXml(movie.genres).c_str()));
        }
        if (movie.homepage != "") {
           s->write(cString::sprintf("    <homepage>%s</homepage>\n", StringExtension::encodeToXml(movie.homepage).c_str()));
        }
        if (movie.releaseDate != "") {
           s->write(cString::sprintf("    <release_date>%s</release_date>\n", StringExtension::encodeToXml(movie.releaseDate).c_str()));
        }
        if (movie.runtime > 0) {
           s->write(cString::sprintf("    <runtime>%i</runtime>\n", movie.runtime));
        }
        if (movie.popularity > 0) {
           s->write(cString::sprintf("    <popularity>%.2f</popularity>\n", movie.popularity));
        }
        if (movie.voteAverage > 0) {
           s->write(cString::sprintf("    <vote_average>%.2f</vote_average>\n", movie.voteAverage));
        }
        if ((movie.poster.width > 0) && (movie.poster.height > 0) && (movie.poster.path.size() > 0)) {
           s->write(cString::sprintf("    <poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.poster.path).c_str(), movie.poster.width, movie.poster.height));
        }
        if ((movie.fanart.width > 0) && (movie.fanart.height > 0) && (movie.fanart.path.size() > 0)) {
           s->write(cString::sprintf("    <fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.fanart.path).c_str(), movie.fanart.width, movie.fanart.height));
        }
        if ((movie.collectionPoster.width > 0) && (movie.collectionPoster.height > 0) && (movie.collectionPoster.path.size() > 0)) {
           s->write(cString::sprintf("    <collection_poster path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.collectionPoster.path).c_str(), movie.collectionPoster.width, movie.collectionPoster.height));
        }
        if ((movie.collectionFanart.width > 0) && (movie.collectionFanart.height > 0) && (movie.collectionFanart.path.size() > 0)) {
           s->write(cString::sprintf("    <collection_fanart path=\"%s\" width=\"%i\" height=\"%i\" />\n",
                                     StringExtension::encodeToXml(movie.collectionFanart.path).c_str(), movie.collectionFanart.width, movie.collectionFanart.height));
        }
        if (movie.actors.size() > 0) {
           int _actors = movie.actors.size();
           for (int i = 0; i < _actors; i++) {
               s->write(cString::sprintf("    <actor name=\"%s\" role=\"%s\" thumb=\"%s\"/>\n",
                                         StringExtension::encodeToXml(movie.actors[i].name).c_str(),
                                         StringExtension::encodeToXml(movie.actors[i].role).c_str(),
                                         StringExtension::encodeToXml(movie.actors[i].actorThumb.path).c_str()));
           }
        }
        s->write("  </param>\n");
     }
  }
  s->write(" </event>\n");
}

void XmlEventList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>\n", Count(), total));
  s->write("</events>");
}
