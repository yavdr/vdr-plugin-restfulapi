#include "events.h"
using namespace std;

void EventsResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET, POST");
      reply.httpReturn(200, "OK");
      return;
  }

  QueryHandler::addHeader(reply);
  if ( (int)request.url().find("/events/image/") == 0 ) {
     replyImage(out, request, reply);
  } else if ( (int)request.url().find("/events/search") == 0 ){
     replySearchResult(out, request, reply);
  }

  else if ( (int)request.url().find("/events/contentdescriptors") == 0 ){
      replyContentDescriptors(out, request, reply);
  }

  else {
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

#if APIVERSNUM > 20300
    LOCK_CHANNELS_READ;
    const cChannels& channels = *Channels;
#else
    cChannels& channels = Channels;
#endif

  const cChannel* channel = VdrExtension::getChannel(channel_id);
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
  if ( channel_to <= 0 || channel != NULL ) channel_to = channels.Count();
 
  if ( from <= -1 ) from = time(NULL); // default time is now
  if ( timespan <= -1 ) timespan = 0; // default timespan is 0, which means all entries will be returned
  int to = from + timespan;

#if APIVERSNUM > 20300
	LOCK_SCHEDULES_READ;
#else
	cSchedulesLock MutexLock;
	const cSchedules *Schedules = cSchedules::Schedules(MutexLock);
#endif

  if( !Schedules ) {
     reply.httpReturn(404, "Could not find schedules!");
     return;
  }


  if ( start_filter >= 0 && limit_filter >= 1 ) {
     eventList->activateLimit(start_filter, limit_filter);
  }

  bool initialized = false;
  int total = 0;
  for(int i=0; i<channels.Count(); i++) {
     const cSchedule *Schedule = Schedules->GetSchedule(channels.Get(i)->GetChannelID());
     
     if ((channel == NULL || strcmp(channel->GetChannelID().ToString(), channels.Get(i)->GetChannelID().ToString()) == 0) && (i >= channel_from && i <= channel_to)) {
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
           for(const cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event)) {
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
  double timediff = -1;
  
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

  if (request.hasHeader("If-Modified-Since")) {
      timediff = difftime(FileExtension::get()->getModifiedTime(path), FileExtension::get()->getModifiedSinceTime(request));
  }

  if (timediff > 0.0 || timediff < 0.0) {
    if ( se.writeBinary(path) ) {
       reply.addHeader("Content-Type", contenttype.c_str());
       FileExtension::get()->addModifiedHeader(path, reply);
    } else {
       reply.httpReturn(404, "Could not find image!");
    }
  } else {
      reply.httpReturn(304, "Not-Modified");
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
  string search = q.getBodyAsString("search");

  if ( query.length() == 0 && search.length() == 0 ) {
     reply.httpReturn(402, "Query required");
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
  eventList->init();
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  int date_filter = q.getOptionAsInt("date_limit");
  if ( start_filter >= 0 && limit_filter >= 1 ) {
     eventList->activateLimit(start_filter, limit_filter);
  }
  if ( date_filter >= 0  ) {
     eventList->activateDateLimit(date_filter);
  }
  
  int total = 0;
  if ( search.length() > 0 ) {

      vdrlive::SearchTimer* searchtimer = new vdrlive::SearchTimer;
      searchtimer->SetId(0);
      string result = searchtimer->LoadCommonFromQuery(q);

      if (result.length() > 0) {
           reply.httpReturn(406, result.c_str());
           return;
      }

      string query = searchtimer->ToText();
      vdrlive::SearchResults* results = new vdrlive::SearchResults;
      results->GetByQuery(query);

      for (vdrlive::SearchResults::iterator result = results->begin(); result != results->end(); ++result) {

          eventList->addEvent(((cEvent*)result->GetEvent()));
          total++;
      }
      delete searchtimer;
      delete results;

  } else {

      int mode = q.getBodyAsInt("mode");// search mode (0=phrase, 1=and, 2=or, 3=exact, 4=regular expression, 5=fuzzy)
      string channelid = q.getBodyAsString("channelid"); //id !!
      bool use_title = q.getBodyAsString("use_title") == "true";
      bool use_subtitle = q.getBodyAsString("use_subtitle") == "true";
      bool use_description = q.getBodyAsString("use_description") == "true";

      int channel = 0;
      const cChannel* channelInstance = VdrExtension::getChannel(channelid);
      if (channelInstance != NULL) {
         channel = channelInstance->Number();
      }

#if APIVERSNUM > 20300
    LOCK_CHANNELS_READ;
    const cChannels& channels = *Channels;
#else
    cChannels& channels = Channels;
#endif

      if (!use_title && !use_subtitle && !use_description)
         use_title = true;
      if (mode < 0 || mode > 5)
         mode = 0;
      if (channel < 0 || channel > channels.Count())
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
      delete epgquery;

  }
  eventList->setTotal(total);
  eventList->finish();
  delete eventList;
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
  si.addMember("parental_rating") <<= e.ParentalRating;
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

#ifdef EPG_DETAILS_PATCH
  si.addMember("details") <<= *e.Details;
#endif

  si.addMember("additional_media") <<= e.AdditionalMedia;

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
  dateLimit = 0;
  Scraper2VdrService sc;
}

void EventList::activateDateLimit(int _limit) {

  if (_limit > 0) {
      dateLimit = _limit;
  }
}

bool EventList::filtered(int start_time) {

  if (dateLimit > 0 && start_time > dateLimit) {
      return true;
  }
  return BaseList::filtered();
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

void HtmlEventList::addEvent(const cEvent* event)
{
  if ( filtered(event->StartTime()) ) return;
  s->write("<li>");
  s->write((char*)event->Title()); //TODO: add more infos
  s->write("\n");
}

void HtmlEventList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonEventList::addEvent(const cEvent* event)
{
  if ( filtered(event->StartTime()) ) return;

  string channelId = StringExtension::toString(event->ChannelID().ToString());
  cxxtools::String eventTitle;
  cxxtools::String eventShortText;
  cxxtools::String eventDescription;
  cxxtools::String empty = StringExtension::UTF8Decode("");
  cxxtools::String channelStr = StringExtension::UTF8Decode(channelId);
  cxxtools::String channelName = StringExtension::UTF8Decode((string)VdrExtension::getChannel(channelId)->Name());

  SerEvent serEvent;

  if( !event->Title() ) { eventTitle = empty; } else { eventTitle = StringExtension::UTF8Decode(event->Title()); }
  if( !event->ShortText() ) { eventShortText = empty; } else { eventShortText = StringExtension::UTF8Decode(event->ShortText()); }
  if( !event->Description() ) { eventDescription = empty; } else { eventDescription = StringExtension::UTF8Decode(event->Description()); }

  SerAdditionalMedia am;
  if (sc.getMedia(event, am)) {
      serEvent.AdditionalMedia = am;
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
  serEvent.ParentalRating = event->ParentalRating();
  serEvent.Vps = event->Vps();
  serEvent.Instance = event;

  const cTimer* timer = VdrExtension::TimerExists(event);
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

  serEvents.push_back(serEvent);
}

void JsonEventList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.beautify();
  serializer.serialize(serEvents.size(), "count");
  serializer.serialize(total, "total");
  serializer.serialize(serEvents, "events");
  serializer.finish();
}

void XmlEventList::init()
{
  s->writeXmlHeader();  
  s->write("<events xmlns=\"http://www.domain.org/restfulapi/2011/events-xml\">\n");
}

void XmlEventList::addEvent(const cEvent* event)
{
  if ( filtered(event->StartTime()) ) return;

  string channelId = StringExtension::toString(event->ChannelID().ToString());
  string eventTitle;
  string eventShortText;
  string eventDescription;

  if ( event->Title() == NULL ) { eventTitle = ""; } else { eventTitle = event->Title(); }
  if ( event->ShortText() == NULL ) { eventShortText = ""; } else { eventShortText = event->ShortText(); }
  if ( event->Description() == NULL ) { eventDescription = ""; } else { eventDescription = event->Description(); }
   
  s->write(" <event>\n");
  s->write(cString::sprintf("  <param name=\"id\">%i</param>\n", event->EventID()));
  s->write(cString::sprintf("  <param name=\"title\">%s</param>\n", StringExtension::encodeToXml(eventTitle).c_str()));
  s->write(cString::sprintf("  <param name=\"short_text\">%s</param>\n", StringExtension::encodeToXml(eventShortText).c_str()));
  s->write(cString::sprintf("  <param name=\"description\">%s</param>\n", StringExtension::encodeToXml(eventDescription).c_str()));

  s->write(cString::sprintf("  <param name=\"channel\">%s</param>\n", StringExtension::encodeToXml(channelId).c_str()));
  s->write(cString::sprintf("  <param name=\"channel_name\">%s</param>\n", StringExtension::encodeToXml((string)VdrExtension::getChannel(channelId)->Name()).c_str()));

  s->write(cString::sprintf("  <param name=\"start_time\">%i</param>\n", (int)event->StartTime()));
  s->write(cString::sprintf("  <param name=\"duration\">%i</param>\n", event->Duration()));
  s->write(cString::sprintf("  <param name=\"table_id\">%i</param>\n", (int)event->TableID()));
  s->write(cString::sprintf("  <param name=\"version\">%i</param>\n", (int)event->Version()));
  s->write(cString::sprintf("  <param name=\"parental_rating\">%i</param>\n", event->ParentalRating()));
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

  const cTimer* timer = VdrExtension::TimerExists(event);
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

  s->write("  <param name=\"contents\">\n");
  int counter = 0;
  uchar content = event->Contents(counter);
  while(content != 0) {
    counter++;
    s->write(cString::sprintf("   <content name=\"%s\" />\n", cEvent::ContentToString(content)));
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

  s->write(cString::sprintf("  <param name=\"timer_exists\">%s</param>\n", (timer_exists ? "true" : "false")));
  s->write(cString::sprintf("  <param name=\"timer_active\">%s</param>\n", (timer_active ? "true" : "false")));
  s->write(cString::sprintf("  <param name=\"timer_id\">%s</param>\n", timer_id.c_str()));

  sc.getMedia(event, s);

  s->write(" </event>\n");
}

void XmlEventList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>\n", Count(), total));
  s->write("</events>");
}

// content strings

void EventsResponder::replyContentDescriptors(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/events/contentdescriptors", request);

  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve content descriptors use the POST method!");
     return;
  }

  StreamExtension se(&out);

  ContentDescriptorList* list;

  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     list = (ContentDescriptorList*)new JsonContentDescriptorList(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     list = (ContentDescriptorList*)new HtmlContentDescriptorList(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     list = (ContentDescriptorList*)new XmlContentDescriptorList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  list->init();
  int total = 0;
  std::set<std::string> contentStrings;

  for(unsigned int i=0; i<CONTENT_DESCRIPTOR_MAX;i++) {

    const string contentDescr = cEvent::ContentToString(i);
    SerContentDescriptor cDescr;
    if (!contentDescr.empty() && contentStrings.find(contentDescr) == contentStrings.end()) {

	contentStrings.insert(contentDescr);
	cDescr.name = StringExtension::UTF8Decode(contentDescr);
	std::stringstream stream;
	stream << std::hex << i;
	std::string result( stream.str() );

	switch (i) {
	  case ecgArtsCulture:
	  case ecgChildrenYouth:
	  case ecgEducationalScience:
	  case ecgLeisureHobbies:
	  case ecgMovieDrama:
	  case ecgMusicBalletDance:
	  case ecgNewsCurrentAffairs:
	  case ecgShow:
	  case ecgSocialPoliticalEconomics:
	  case ecgSpecial:
	  case ecgSports:
	  case ecgUserDefined:
	      cDescr.isGroup = true;
	      break;
	  default:
	      cDescr.isGroup = false;
	}

	cDescr.id = result;
	list->addDescr(cDescr);
	total++;
    }
  }

  list->setTotal(total);
  list->finish();
  delete list;
};

ContentDescriptorList::ContentDescriptorList(std::ostream* _out)
{
  s = new StreamExtension(_out);
  total = 0;
}

ContentDescriptorList::~ContentDescriptorList()
{
  delete s;
}

void HtmlContentDescriptorList::init()
{
  s->writeHtmlHeader( "Content descriptors" );
  s->write("<ul>");
}

void HtmlContentDescriptorList::addDescr(SerContentDescriptor &descr)
{
  if ( filtered() ) return;

  s->write(cString::sprintf("<li>%s - %s</li>", StringExtension::encodeToXml(descr.id).c_str(), StringExtension::encodeToXml(descr.name).c_str()));
}

void HtmlContentDescriptorList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonContentDescriptorList::addDescr(SerContentDescriptor &descr)
{
  if ( filtered() ) return;
  descr.id = StringExtension::encodeToJson(descr.id);
  descr.name = StringExtension::encodeToJson(descr.name);
  serContentDescriptors.push_back(descr);
}

void JsonContentDescriptorList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(serContentDescriptors, "content_descriptors");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlContentDescriptorList::init()
{
  s->writeXmlHeader();
  s->write("<lists xmlns=\"http://www.domain.org/restfulapi/2011/groups-xml\">\n");
}

void XmlContentDescriptorList::addDescr(SerContentDescriptor &descr)
{
  if ( filtered() ) return;
  s->write(cString::sprintf(" <content_descriptor>\n"));
  s->write(cString::sprintf("  <id>%s</id>\n", StringExtension::encodeToXml(descr.id).c_str()));
  s->write(cString::sprintf("  <name>%s</name>\n", StringExtension::encodeToXml(descr.name).c_str()));
  s->write(cString::sprintf("  <is_group>%s</is_group>\n", descr.isGroup ? "true" : "false"));
  s->write(cString::sprintf(" </content_descriptor>\n"));
}

void XmlContentDescriptorList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</lists>");
}

void operator<<= (cxxtools::SerializationInfo& si, const SerContentDescriptor& t)
{
  si.addMember("id") <<= t.id;
  si.addMember("name") <<= t.name;
  si.addMember("is_group") <<= t.isGroup;
}

