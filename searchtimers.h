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
#include "epgsearch.h"
#include "events.h"

#ifndef __RESTFUL_SEARCHTIMERS_H
#define __RESETFUL_SEARCHTIMERS_H

class SearchTimersResponder : public cxxtools::http::Responder
{
  public:
    explicit SearchTimersResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyShow(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyCreate(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyDelete(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replySearch(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<SearchTimersResponder> SearchTimersService;

class SearchTimerList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public: 
    SearchTimerList(std::ostream* _out);
    ~SearchTimerList();
    virtual void init() { };
    virtual void addSearchTimer(SerSearchTimerContainer searchTimer) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; };
};

class HtmlSearchTimerList : public SearchTimerList
{
  public:
    HtmlSearchTimerList(std::ostream* _out) : SearchTimerList(_out) { };
    ~HtmlSearchTimerList() { };
    virtual void init();
    virtual void addSearchTimer(SerSearchTimerContainer searchTimer);
    virtual void finish();
};

class JsonSearchTimerList : public SearchTimerList
{
  private:
    std::vector< SerSearchTimerContainer > _items;
  public:
    JsonSearchTimerList(std::ostream* _out) : SearchTimerList(_out) { };
    ~JsonSearchTimerList() { };
    virtual void init() { };
    virtual void addSearchTimer(SerSearchTimerContainer searchTimer);
    virtual void finish();
};

class XmlSearchTimerList : public SearchTimerList
{
  public:
    XmlSearchTimerList(std::ostream* _out) : SearchTimerList(_out) { };
    ~XmlSearchTimerList() { };
    virtual void init();
    virtual void addSearchTimer(SerSearchTimerContainer searchTimer);
    virtual void finish();
};

#endif
