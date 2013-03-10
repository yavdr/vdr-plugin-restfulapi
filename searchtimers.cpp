#include "searchtimers.h"
using namespace std;

void SearchTimersResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  cPlugin* plugin = cPluginManager::GetPlugin("epgsearch");
  if (plugin == NULL) {
     reply.httpReturn(403, "Epgsearch isn't installed!");
     return; 
  }

  if ((int)request.url().find("/searchtimers/search/") == 0 ) {
     replySearch(out, request, reply);
  } else { 
     if (request.method() == "GET") {
        replyShow(out, request, reply);
     } else if (request.method() == "POST") {
        replyCreate(out, request, reply);
     } else if (request.method() == "DELETE") {
        replyDelete(out, request, reply);
     } else if (request.method() == "OPTIONS") {
        return;	
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

