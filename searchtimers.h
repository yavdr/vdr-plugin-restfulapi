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
#include "channels.h"

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
    virtual void replyChannelGroups(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyRecordingDirs(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyBlacklists(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyTimerConflicts(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
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

class RecordingDirsList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    explicit RecordingDirsList(std::ostream* _out);
    virtual ~RecordingDirsList();
    virtual void init() { };
    virtual void addDir(std::string dir) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlRecordingDirsList : RecordingDirsList
{
  public:
    explicit HtmlRecordingDirsList(std::ostream* _out) : RecordingDirsList(_out) { };
    ~HtmlRecordingDirsList() { };
    virtual void init();
    virtual void addDir(std::string dir);
    virtual void finish();
};

class JsonRecordingDirsList : RecordingDirsList
{
  private:
    std::vector< cxxtools::String > dirs;
  public:
    explicit JsonRecordingDirsList(std::ostream* _out) : RecordingDirsList(_out) { };
    ~JsonRecordingDirsList() { };
    virtual void init() { };
    virtual void addDir(std::string dir);
    virtual void finish();
};

class XmlRecordingDirsList : RecordingDirsList
{
  public:
    explicit XmlRecordingDirsList(std::ostream* _out) : RecordingDirsList(_out) { };
    ~XmlRecordingDirsList() { };
    virtual void init();
    virtual void addDir(std::string dir);
    virtual void finish();
};

class Blacklists : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    explicit Blacklists(std::ostream* _out);
    virtual ~Blacklists();
    virtual void init() { };
    virtual void addList(int list) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlBlacklists : Blacklists
{
  public:
    explicit HtmlBlacklists(std::ostream* _out) : Blacklists(_out) { };
    ~HtmlBlacklists() { };
    virtual void init();
    virtual void addList(int list);
    virtual void finish();
};

class JsonBlacklists : Blacklists
{
  private:
    std::vector< int > blacklists;
  public:
    explicit JsonBlacklists(std::ostream* _out) : Blacklists(_out) { };
    ~JsonBlacklists() { };
    virtual void init() { };
    virtual void addList(int list);
    virtual void finish();
};

class XmlBlacklists : Blacklists
{
  public:
    explicit XmlBlacklists(std::ostream* _out) : Blacklists(_out) { };
    ~XmlBlacklists() { };
    virtual void init();
    virtual void addList(int list);
    virtual void finish();
};

class TimerConflicts : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    explicit TimerConflicts(std::ostream* _out);
    virtual ~TimerConflicts();
    virtual void init(bool checkAdvised) { };
    virtual void addConflict(std::string conflict) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlTimerConflicts : TimerConflicts
{
  public:
    explicit HtmlTimerConflicts(std::ostream* _out) : TimerConflicts(_out) { };
    ~HtmlTimerConflicts() { };
    virtual void init(bool checkAdvised);
    virtual void addConflict(std::string conflict);
    virtual void finish();
};

class JsonTimerConflicts : TimerConflicts
{
  private:
    std::vector< std::string > conflicts;
    bool isCheckAdvised;
  public:
    explicit JsonTimerConflicts(std::ostream* _out) : TimerConflicts(_out) { isCheckAdvised = false; };
    ~JsonTimerConflicts() { };
    virtual void init(bool checkAdvised);
    virtual void addConflict(std::string conflict);
    virtual void finish();
};

class XmlTimerConflicts : TimerConflicts
{
  public:
    explicit XmlTimerConflicts(std::ostream* _out) : TimerConflicts(_out) { };
    ~XmlTimerConflicts() { };
    virtual void init(bool checkAdvised);
    virtual void addConflict(std::string conflict);
    virtual void finish();
};

#endif
