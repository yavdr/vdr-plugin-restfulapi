#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/epg.h>
#include <vdr/menu.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/query_params.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/regex.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include <iostream>
#include <sstream>
#include <stack>
#include <time.h>
#include "tools.h"


class TimersResponder : public cxxtools::http::Responder
{
  public:
    explicit TimersResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
     virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
     void createOrUpdateTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, bool update);
     void deleteTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
     void showTimers(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<TimersResponder> TimersService;

struct SerTimer
{
  int Id;
  int Start;
  int Stop;
  int Priority;
  int Lifetime;
  int EventID;
  int WeekDays;
  int Day;
  int Channel;
  bool IsRecording;
  bool IsPending;
  bool IsActive;
  cxxtools::String FileName;
  cxxtools::String ChannelName;
};

struct SerTimers
{
  std::vector < struct SerTimer > timer;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerTimer& t);
void operator>>= (const cxxtools::SerializationInfo& si, SerTimer& t);
void operator<<= (cxxtools::SerializationInfo& si, const SerTimers& t);

class TimerList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    TimerList(std::ostream* _out);
    ~TimerList();
    virtual void init() { };
    virtual void addTimer(cTimer* timer, int nr) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlTimerList : TimerList
{
  public:
    HtmlTimerList(std::ostream* _out) : TimerList(_out) { };
    ~HtmlTimerList() { };
    virtual void init();
    virtual void addTimer(cTimer* timer, int nr);
    virtual void finish();
};

class JsonTimerList : TimerList
{
  private:
    std::vector < struct SerTimer > serTimers;
  public:
    JsonTimerList(std::ostream* _out) : TimerList(_out) { };
    ~JsonTimerList() { };
    virtual void addTimer(cTimer* timer, int nr);
    virtual void finish();
};

class XmlTimerList : TimerList
{
  public:
    XmlTimerList(std::ostream* _out) : TimerList(_out) { };
    ~XmlTimerList() { };
    virtual void init();
    virtual void addTimer(cTimer* timer, int nr);
    virtual void finish();
};

class TimerValues
{
  private:
    std::stack<int>  ConvertToBinary(int v);
  public:
    TimerValues() { };
    ~TimerValues() { };
    bool	IsDayValid(std::string v);
    bool 	IsFlagsValid(int v);
    bool	IsFileValid(std::string v);
    bool	IsLifetimeValid(int v);
    bool	IsPriorityValid(int v);
    bool	IsStopValid(int v);
    bool	IsStartValid(int v);
    bool	IsWeekdaysValid(std::string v);

    int		ConvertFlags(std::string v);
    cEvent*	ConvertEvent(std::string event_id, cChannel* channel);
    std::string ConvertFile(std::string v); // replaces : with | - required by parsing method of VDR
    std::string ConvertAux(std::string v);  // replaces : with | - required by parsing method of VDR
    int		ConvertLifetime(std::string v);
    int		ConvertPriority(std::string v);
    int		ConvertStop(std::string v);
    int		ConvertStart(std::string v);
    int		ConvertDay(std::string v);
    std::string ConvertDay(time_t v);
    std::string ConvertWeekdays(int v);
    int		ConvertWeekdays(std::string v);
    cChannel*	ConvertChannel(std::string v);
    cTimer*	ConvertTimer(std::string v);
};
