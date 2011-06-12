#include <iostream>
#include <fstream>
#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include "tools.h"

#include <vdr/epg.h>

class EventsResponder : public cxxtools::http::Responder
{
  public:
    explicit EventsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyEvents(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyImage(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<EventsResponder> EventsService;

struct SerEvent
{
  int Id;
  cxxtools::String Title;
  cxxtools::String ShortText;
  cxxtools::String Description;
  int StartTime;
  int Duration;
  cxxtools::String* Images;
  int ImagesCount;
};

struct SerEvents
{
  std::vector < struct SerEvent > event;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerEvent& e);
void operator<<= (cxxtools::SerializationInfo& si, const SerEvents& e);

class EventList
{
  protected:
    StreamExtension *s;
  public:
    EventList(std::ostream* _out);
    ~EventList();
    virtual void init() { };
    virtual void addEvent(cEvent* event, bool scan_images) { };
    virtual void finish() { };
};

class HtmlEventList : EventList
{
  public:
    HtmlEventList(std::ostream* _out) : EventList(_out) { };
    ~HtmlEventList() { };
    virtual void init();
    virtual void addEvent(cEvent* event, bool scan_images);
    virtual void finish();
};

class JsonEventList : EventList
{
  private:
    std::vector < struct SerEvent > serEvents;
  public:
    JsonEventList(std::ostream* _out) : EventList(_out) { };
    ~JsonEventList() { };
    virtual void addEvent(cEvent* event, bool scan_images);
    virtual void finish();
};

class XmlEventList : EventList
{
  public:
    XmlEventList(std::ostream* _out) : EventList(_out) { };
    ~XmlEventList() { };
    virtual void init();
    virtual void addEvent(cEvent* event, bool scan_images);
    virtual void finish();
};
