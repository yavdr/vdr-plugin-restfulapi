#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/query_params.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include "tools.h"

#include "vdr/timers.h"
#include "vdr/menu.h"

class TimersResponder : public cxxtools::http::Responder
{
  public:
    explicit TimersResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
     virtual void reply(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
     virtual void createTimer(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
     virtual void deleteTimer(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
     virtual void showTimers(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<TimersResponder> TimersService;

struct SerTimer
{
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

class TimerList
{
  protected:
    std::ostream* out;
  public:
    TimerList(std::ostream* _out) { out = _out; };
    ~TimerList() { };
    virtual void init() { };
    virtual void addTimer(cTimer* timer) { };
    virtual void finish() { };
};

class HtmlTimerList : TimerList
{
  public:
    HtmlTimerList(std::ostream* _out) : TimerList(_out) { };
    ~HtmlTimerList() { };
    virtual void init();
    virtual void addTimer(cTimer* timer);
    virtual void finish();
};

class JsonTimerList : TimerList
{
  private:
    std::vector < struct SerTimer > serTimers;
  public:
    JsonTimerList(std::ostream* _out) : TimerList(_out) { };
    ~JsonTimerList() { };
    virtual void addTimer(cTimer* timer);
    virtual void finish();
};
