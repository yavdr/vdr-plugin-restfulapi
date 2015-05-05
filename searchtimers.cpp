#include "searchtimers.h"
using namespace std;

void SearchTimersResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET, POST, DELETE");
      reply.httpReturn(200, "OK");
      return;
  }

  cPlugin* plugin = cPluginManager::GetPlugin("epgsearch");
  if (plugin == NULL) {
     reply.httpReturn(403, "Epgsearch isn't installed!");
     return; 
  }

  if ((int)request.url().find("/searchtimers/search/") == 0 ) {
     replySearch(out, request, reply);
  } else if (request.method() == "GET" && (int)request.url().find("/searchtimers/channelgroups") == 0 ) {
      replyChannelGroups(out, request, reply);
  } else if (request.method() == "GET" && (int)request.url().find("/searchtimers/recordingdirs") == 0 ) {
      replyRecordingDirs(out, request, reply);
  } else if (request.method() == "GET" && (int)request.url().find("/searchtimers/blacklists") == 0 ) {
      replyBlacklists(out, request, reply);
  } else if (request.method() == "GET" && (int)request.url().find("/searchtimers/conflicts") == 0 ) {
      replyTimerConflicts(out, request, reply);
  } else if (request.method() == "GET" && (int)request.url().find("/searchtimers/extepginfo") == 0 ) {
      replyExtEpgInfo(out, request, reply);
  } else if (request.method() == "POST" && (int)request.url().find("/searchtimers/update") == 0 ) {
      replyTriggerUpdate(out, request, reply);
  } else { 
     if (request.method() == "GET") {
        replyShow(out, request, reply);
     } else if (request.method() == "POST") {
        replyCreate(out, request, reply);
     } else if (request.method() == "DELETE") {
        replyDelete(out, request, reply);
     } else {
        reply.httpReturn(404, "The searchtimer-service does only support the following methods: GET, POST and DELETE.");
     }
  }
}

void SearchTimersResponder::replyShow(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/searchtimers", request);
  vdrlive::SearchTimers service;
  SearchTimerList* stList;

  if (q.isFormat(".json")) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     stList = (SearchTimerList*)new JsonSearchTimerList(&out);
  } else if ( q.isFormat(".html")) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     stList = (SearchTimerList*)new HtmlSearchTimerList(&out);
  } else if ( q.isFormat(".xml")) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     stList = (SearchTimerList*)new XmlSearchTimerList(&out);
  } else {
     reply.httpReturn(405, "Resources are not available for the selected format. (Use: .json, .html or .xml)");
     return;
  }

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  if ( start_filter >= 0 && limit_filter >= 1 ) stList->activateLimit(start_filter, limit_filter);

  stList->init();
  int counter = 0;

  for(vdrlive::SearchTimers::iterator timer = service.begin(); timer != service.end(); ++timer)
  { 
    SerSearchTimerContainer container;
    container.timer = &(*timer);
    stList->addSearchTimer(container);
    counter++;
  }

  stList->setTotal(counter);
  stList->finish();
  
  delete stList;
}

void SearchTimersResponder::replyCreate(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/searchtimers", request);
  vdrlive::SearchTimer* searchTimer = new vdrlive::SearchTimer();
  vdrlive::SearchTimers searchTimers;
  string result = searchTimer->LoadFromQuery(q);

  if (result.length() > 0)
  { 
     reply.httpReturn(406, result.c_str());
  } else {
     bool succeeded = searchTimers.Save(searchTimer);
     if(succeeded) {
        reply.httpReturn(200, (const char*)cString::sprintf("OK, Id:%i", searchTimer->Id()));
     } else {
        reply.httpReturn(407, "Creating searchtimer failed.");
     }
  }

  delete searchTimer;
}

void SearchTimersResponder::replyDelete(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/searchtimers", request);
  vdrlive::SearchTimers searchTimers;
  string id = q.getParamAsString(0);
  bool result = searchTimers.Delete(id);

  if (!result)
     reply.httpReturn(408, "Deleting searchtimer failed!");
  else
     reply.httpReturn(200, "Searchtimer deleted.");  
}

void SearchTimersResponder::replySearch(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/searchtimers/search", request);
  vdrlive::SearchResults searchResults;
  int id = q.getParamAsInt(0);

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
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }
  
  searchResults.GetByID(id);

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");
  if ( start_filter >= 0 && limit_filter >= 1 )
     eventList->activateLimit(start_filter, limit_filter);
  
  eventList->init();
  int total = 0;
  
  for (vdrlive::SearchResults::iterator item = searchResults.begin(); item != searchResults.end(); ++item) {
    eventList->addEvent((cEvent*)item->GetEvent());
    total++;
  }

  eventList->setTotal(total);
  eventList->finish();
  delete eventList;
}

void SearchTimersResponder::replyTriggerUpdate(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/searchtimers/update", request);
  vdrlive::SearchTimers service;
  service.TriggerUpdate();
  reply.httpReturn(200, "OK - Update triggered");
};

SearchTimerList::SearchTimerList(ostream* _out)
{
  s = new StreamExtension(_out);
  total = 0;
}

SearchTimerList::~SearchTimerList()
{
  delete s;
}

void HtmlSearchTimerList::init()
{
  s->writeHtmlHeader("HtmlSearchTimerList");
  s->write("<ul>");
}

void HtmlSearchTimerList::addSearchTimer(SerSearchTimerContainer searchTimer)
{
  if ( filtered() ) return;
  s->write(searchTimer.timer->ToHtml().c_str());
}

void HtmlSearchTimerList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonSearchTimerList::addSearchTimer(SerSearchTimerContainer searchTimer)
{
  if ( filtered() ) return;
  _items.push_back(searchTimer);
}

void JsonSearchTimerList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(_items, "searchtimers");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlSearchTimerList::init()
{
  s->writeXmlHeader();
  s->write("<searchtimers xmlns=\"http://www.domain.org/restfulapi/2011/searchtimers-xml\" >\n");
}

void XmlSearchTimerList::addSearchTimer(SerSearchTimerContainer searchTimer)
{
  if ( filtered() ) return;
  s->write(searchTimer.timer->ToXml().c_str());
}

void XmlSearchTimerList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>\n", Count(), total));
  s->write("</searchtimers>\n");
}


// recordingdirs responder

void SearchTimersResponder::replyRecordingDirs(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/searchtimers/recordingdirs", request);
  RecordingDirsList* dirsList;
  vdrlive::RecordingDirs recordingDirs;


  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     dirsList = (RecordingDirsList*)new JsonRecordingDirsList(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     dirsList = (RecordingDirsList*)new HtmlRecordingDirsList(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     dirsList = (RecordingDirsList*)new XmlRecordingDirsList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  dirsList->init();
  int total = 0;

  for (vdrlive::RecordingDirs::iterator item = recordingDirs.begin(); item != recordingDirs.end(); ++item) {

      //esyslog("restful: dir: %s", (std::string)item);

      dirsList->addDir(*item);
      total++;
  }

  dirsList->setTotal(total);
  dirsList->finish();
  delete dirsList;

}

RecordingDirsList::RecordingDirsList(std::ostream* _out)
{
  s = new StreamExtension(_out);
  total = 0;
}

RecordingDirsList::~RecordingDirsList()
{
  delete s;
}

void HtmlRecordingDirsList::init()
{
  s->writeHtmlHeader( "HtmlRecordingDirsList" );
  s->write("<ul>");
}

void HtmlRecordingDirsList::addDir(string dir)
{
  if ( filtered() ) return;

  s->write(cString::sprintf("<li>%s</li>", dir.c_str()));
}

void HtmlRecordingDirsList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonRecordingDirsList::addDir(string dir)
{
  if ( filtered() ) return;
  dirs.push_back(StringExtension::UTF8Decode(dir));
}

void JsonRecordingDirsList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(dirs, "dirs");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlRecordingDirsList::init()
{
  s->writeXmlHeader();
  s->write("<dirs xmlns=\"http://www.domain.org/restfulapi/2011/groups-xml\">\n");
}

void XmlRecordingDirsList::addDir(string dir)
{
  if ( filtered() ) return;
  s->write(cString::sprintf(" <dir>%s</dir>\n", StringExtension::encodeToXml( dir ).c_str()));
}

void XmlRecordingDirsList::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</dirs>");
}


// Blacklists responder

void SearchTimersResponder::replyBlacklists(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/searchtimers/blacklists", request);
  Blacklists* list;
  vdrlive::Blacklists blacklists;


  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     list = (Blacklists*)new JsonBlacklists(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     list = (Blacklists*)new HtmlBlacklists(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     list = (Blacklists*)new XmlBlacklists(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  list->init();
  int total = 0;

  for (vdrlive::Blacklists::iterator item = blacklists.begin(); item != blacklists.end(); ++item) {

      Blacklist bList;
      bList.search = item->Search();
      bList.id = item->Id();
      list->addList(bList);
      total++;
  }

  list->setTotal(total);
  list->finish();
  delete list;

}

Blacklists::Blacklists(std::ostream* _out)
{
  s = new StreamExtension(_out);
  total = 0;
}

Blacklists::~Blacklists()
{
  delete s;
}

void HtmlBlacklists::init()
{
  s->writeHtmlHeader( "Blacklists" );
  s->write("<ul>");
}

void HtmlBlacklists::addList(Blacklist list)
{
  if ( filtered() ) return;

  s->write(cString::sprintf("<li>%d - %s</li>", list.id, list.search.c_str()));
}

void HtmlBlacklists::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonBlacklists::addList(Blacklist list)
{
  if ( filtered() ) return;
  blacklists.push_back(list);
}

void JsonBlacklists::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(blacklists, "blacklists");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlBlacklists::init()
{
  s->writeXmlHeader();
  s->write("<lists xmlns=\"http://www.domain.org/restfulapi/2011/groups-xml\">\n");
}

void XmlBlacklists::addList(Blacklist list)
{
  if ( filtered() ) return;
  s->write(cString::sprintf(" <list>\n"));
  s->write(cString::sprintf("  <id>%d</id>\n", list.id));
  s->write(cString::sprintf("  <search>%s</search>\n", StringExtension::encodeToXml(list.search).c_str()));
  s->write(cString::sprintf(" </list>\n"));
}

void XmlBlacklists::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</lists>");
}

void operator<<= (cxxtools::SerializationInfo& si, const Blacklist& t)
{
  si.addMember("id") <<= t.id;
  si.addMember("search") <<= t.search;
}

// channlegroups responder


void SearchTimersResponder::replyChannelGroups(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/searchtimers/channelgroups", request);
  ChannelGroupList* groupList;
  vdrlive::ChannelGroups channelGroups;


  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     groupList = (ChannelGroupList*)new JsonChannelGroupList(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     groupList = (ChannelGroupList*)new HtmlChannelGroupList(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     groupList = (ChannelGroupList*)new XmlChannelGroupList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  groupList->init();
  int total = 0;

  for (vdrlive::ChannelGroups::iterator item = channelGroups.begin(); item != channelGroups.end(); ++item) {
      groupList->addGroup(item->Name());
    total++;
  }

  groupList->setTotal(total);
  groupList->finish();
  delete groupList;

}

// Timerconflicts responder

void SearchTimersResponder::replyTimerConflicts(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/searchtimers/conflicts", request);
  TimerConflicts* list;
  vdrlive::TimerConflicts conflicts;

  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     list = (TimerConflicts*)new JsonTimerConflicts(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     list = (TimerConflicts*)new HtmlTimerConflicts(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     list = (TimerConflicts*)new XmlTimerConflicts(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  list->init(conflicts.CheckAdvised());
  int total = 0;

  for (vdrlive::TimerConflicts::iterator item = conflicts.begin(); item != conflicts.end(); ++item) {

      list->addConflict(*item);
      total++;
  }

  list->setTotal(total);
  list->finish();
  delete list;

}

TimerConflicts::TimerConflicts(std::ostream* _out)
{
  s = new StreamExtension(_out);
  total = 0;
}

TimerConflicts::~TimerConflicts()
{
  delete s;
}

void HtmlTimerConflicts::init(bool checkAdvised)
{
  s->writeHtmlHeader( "Blacklists" );
  s->write(cString::sprintf("<p>Check advised: %s</p>", checkAdvised ? "true" : "false"));
  s->write("<ul>");
}

void HtmlTimerConflicts::addConflict(std::string conflict)
{
  if ( filtered() ) return;

  s->write(cString::sprintf("<li>%s</li>", conflict.c_str()));
}

void HtmlTimerConflicts::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}


void JsonTimerConflicts::init(bool checkAdvised) {

  isCheckAdvised = checkAdvised;
}

void JsonTimerConflicts::addConflict(std::string conflict)
{
  if ( filtered() ) return;
  conflicts.push_back(conflict);
}

void JsonTimerConflicts::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(isCheckAdvised, "check_advised");
  serializer.serialize(conflicts, "conflicts");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlTimerConflicts::init(bool checkAdvised)
{
  s->writeXmlHeader();
  s->write("<conflicts xmlns=\"http://www.domain.org/restfulapi/2011/groups-xml\">\n");
  s->write(cString::sprintf(" <check_advised>%s</check_advised>\n", checkAdvised ? "true" : "false"));
}

void XmlTimerConflicts::addConflict(string conflict)
{
  if ( filtered() ) return;
  s->write(cString::sprintf(" <conflict>%s</conflict>\n", StringExtension::encodeToXml( conflict ).c_str()));
}

void XmlTimerConflicts::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</conflicts>");
}

// Timerconflicts responder

void SearchTimersResponder::replyExtEpgInfo(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler q("/searchtimers/extepginfo", request);
  ExtEpgInfos* list;
  vdrlive::ExtEPGInfos extepginfos;

  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     list = (ExtEpgInfos*)new JsonExtEpgInfos(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     list = (ExtEpgInfos*)new HtmlExtEpgInfos(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     list = (ExtEpgInfos*)new XmlExtEpgInfos(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json, .xml or .html)");
     return;
  }

  list->init();
  int total = 0;

  for (vdrlive::ExtEPGInfos::iterator item = extepginfos.begin(); item != extepginfos.end(); ++item) {

      ExtEpgInfo info;
      info.name = item->Name();
      info.id = item->Id();
      info.values = item->Values();
      info.config = item->Get();

      list->addInfo(info);
      total++;
  }

  list->setTotal(total);
  list->finish();
  delete list;

}

ExtEpgInfos::ExtEpgInfos(std::ostream* _out)
{
  s = new StreamExtension(_out);
  total = 0;
}

ExtEpgInfos::~ExtEpgInfos()
{
  delete s;
}

void HtmlExtEpgInfos::init()
{
  s->writeHtmlHeader( "Ext EPG Info" );
  s->write("<ul>");
}

void HtmlExtEpgInfos::addInfo(ExtEpgInfo info)
{
  if ( filtered() ) return;

  s->write(cString::sprintf("<li>%s</li>", info.config.c_str()));
}

void HtmlExtEpgInfos::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}


void JsonExtEpgInfos::addInfo(ExtEpgInfo info)
{
  if ( filtered() ) return;
  infos.push_back(info);
}

void JsonExtEpgInfos::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(infos, "ext_epg_info");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlExtEpgInfos::init()
{
  s->writeXmlHeader();
  s->write("<ext_epg_infos xmlns=\"http://www.domain.org/restfulapi/2011/groups-xml\">\n");
}

void XmlExtEpgInfos::addInfo(ExtEpgInfo info)
{
  if ( filtered() ) return;
  s->write(cString::sprintf(" <ext_epg_info>\n"));
  s->write(cString::sprintf(" <ext_epg_info>%d</ext_epg_info>\n", info.id));
  s->write(cString::sprintf(" <name>%s</name>\n", StringExtension::encodeToXml( info.name ).c_str()));


  s->write(cString::sprintf(" <values>\n"));
  vector< string >::const_iterator value = info.values.begin();
  for ( int i = 0; value != info.values.end(); ++i, ++value ) {
      s->write(cString::sprintf(" <value>%s</value>\n", StringExtension::encodeToXml( info.values[i] ).c_str()));
  }


  s->write(cString::sprintf(" </values>\n"));

  s->write(cString::sprintf(" <config>%s</config>\n", StringExtension::encodeToXml( info.config ).c_str()));
  s->write(cString::sprintf(" </ext_epg_info>\n"));
}

void XmlExtEpgInfos::finish()
{
  s->write(cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</ext_epg_infos>");
}

void operator<<= (cxxtools::SerializationInfo& si, const ExtEpgInfo& t)
{
  si.addMember("id") <<= t.id;
  si.addMember("name") <<= t.name;
  si.addMember("values") <<= t.values;
  si.addMember("config") <<= t.config;
}


