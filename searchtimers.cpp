#include "searchtimers.h"

void SearchTimersResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  cPlugin* plugin = cPluginManager::GetPlugin("epgsearch");
  if (plugin == NULL) {
     reply.httpReturn(403, "Epgsearch isn't installed!");
     return; 
  } 
  
  if (request.method() != "GET") {
     reply.httpReturn(404, "To retrieve intformation use the GET method!");
     return;
  }

  replyShow(out, request, reply);
}

void SearchTimersResponder::replyShow(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
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

SearchTimerList::SearchTimerList(std::ostream* _out)
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
  s->writeHtmlHeader();
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
  s->write("</searchtimers>\n");
}

