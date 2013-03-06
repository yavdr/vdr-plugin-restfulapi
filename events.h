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

struct SerComponent
{
  int Stream;
  int Type;
  cxxtools::String Language;
  cxxtools::String Description;
};

struct SerEvent
{
  int Id;
  cxxtools::String Title;
  cxxtools::String ShortText;
  cxxtools::String Description;
  cxxtools::String Channel;
  cxxtools::String ChannelName;
  int StartTime;
  int Duration;
  int TableID;
  int Version;
  int Images;
  bool TimerExists;
  bool TimerActive;
#if APIVERSNUM > 10710 || EPGHANDLER
  int ParentalRating;
#endif
  int Vps;
  cxxtools::String TimerId;
  cEvent* Instance;
#ifdef EPG_DETAILS_PATCH
  std::vector< tEpgDetail >* Details;
#endif
};

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e);
void operator<<= (cxxtools::SerializationInfo& si, const SerComponent& c);
#ifdef EPG_DETAILS_PATCH
void operator<<= (cxxtools::SerializationInfo& si, const struct tEpgDetail& e);
#endif

class EventList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    explicit EventList(std::ostream* _out);
    virtual ~EventList();
    virtual void init() { };
    virtual void addEvent(cEvent* event) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlEventList : EventList
{
  public:
    explicit HtmlEventList(std::ostream* _out) : EventList(_out) { };
    ~HtmlEventList() { };
    virtual void init();
    virtual void addEvent(cEvent* event);
    virtual void finish();
};

class JsonEventList : EventList
{
  private:
    std::vector < struct SerEvent > serEvents;
  public:
    explicit JsonEventList(std::ostream* _out) : EventList(_out) { };
    ~JsonEventList() { };
    virtual void addEvent(cEvent* event);
    virtual void finish();
};

class XmlEventList : EventList
{
  public:
    explicit XmlEventList(std::ostream* _out) : EventList(_out) { };
    ~XmlEventList() { };
    virtual void init();
    virtual void addEvent(cEvent* event);
    virtual void finish();
};

#endif //__RESTFUL_EVENTS_H
