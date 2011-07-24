#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include <vdr/epg.h>
#include <vdr/plugin.h>

#include "tools.h"
#include "epgsearch/services.h"

#ifndef __RESTFUL_EVENTS_H
#define __RESTFUL_EVENTS_H

class EventsResponder : public cxxtools::http::Responder
{
  public:
    explicit EventsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyEvents(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyImage(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replySearchResult(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<EventsResponder> EventsService;

struct SerEvent
{
  int Id;
  cxxtools::String Title;
  cxxtools::String ShortText;
  cxxtools::String Description;
  cxxtools::String Channel;
  int StartTime;
  int Duration;
  int Images;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e);

class EventList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    EventList(std::ostream* _out);
    ~EventList();
    virtual void init() { };
    virtual void addEvent(cEvent* event, cChannel* channel) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlEventList : EventList
{
  public:
    HtmlEventList(std::ostream* _out) : EventList(_out) { };
    ~HtmlEventList() { };
    virtual void init();
    virtual void addEvent(cEvent* event, cChannel* channel);
    virtual void finish();
};

class JsonEventList : EventList
{
  private:
    std::vector < struct SerEvent > serEvents;
  public:
    JsonEventList(std::ostream* _out) : EventList(_out) { };
    ~JsonEventList() { };
    virtual void addEvent(cEvent* event, cChannel* channel);
    virtual void finish();
};

class XmlEventList : EventList
{
  public:
    XmlEventList(std::ostream* _out) : EventList(_out) { };
    ~XmlEventList() { };
    virtual void init();
    virtual void addEvent(cEvent* event, cChannel* channel);
    virtual void finish();
};

#endif //__RESTFUL_EVENTS_H
