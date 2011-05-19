#include "timers.h"

void TimersResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string qparams = request.qparams();
  TimerList* timerList;

  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve information use the GET method!");
     return;
  }

  if ( isFormat(qparams, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     timerList = (TimerList*)new JsonTimerList(&out);
  } else if ( isFormat(qparams, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     timerList = (TimerList*)new HtmlTimerList(&out);
  } else {
     reply.httpReturn(403, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  int timer_count = Timers.Count();
  cTimer *timer;
  for (int i=0;i<timer_count;i++)
  {
     timer = Timers.Get(i);
     timerList->addTimer(timer);   
  }

  if ( isFormat(qparams, ".json") ) {
     delete (JsonTimerList*)timerList;
  } else {
     delete (HtmlTimerList*)timerList;
  }
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTimer& t)
{
  si.addMember("start") <<= t.Start;
  si.addMember("stop") <<= t.Stop;
  si.addMember("priority") <<= t.Priority;
  si.addMember("lifetime") <<= t.Lifetime;
  si.addMember("event_id") <<= t.EventID;
  si.addMember("weekdays") <<= t.WeekDays;
  si.addMember("day") <<= t.Day;
  si.addMember("channel") <<= t.Channel;
  si.addMember("filename") <<= t.FileName;
  si.addMember("is_pending") <<= t.IsPending;
  si.addMember("is_recording") <<= t.IsRecording;
}

void operator>>= (const cxxtools::SerializationInfo& si, SerTimer& t)
{
  si.getMember("start") >>= t.Start;
  si.getMember("stop") >>= t.Stop;
  si.getMember("priority") >>= t.Priority;
  si.getMember("lifetime") >>= t.Lifetime;
  si.getMember("event_id") >>= t.EventID;
  si.getMember("weekdays") >>= t.WeekDays;
  si.getMember("day") >>= t.Day;
  si.getMember("channel") >>= t.Channel;
  si.getMember("filename") >>= t.FileName;
  si.getMember("is_pending") >>= t.IsPending;
  si.getMember("is_recording") >>= t.IsRecording;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTimers& t)
{
  si.addMember("rows") <<= t.timer;
}

HtmlTimerList::HtmlTimerList(std::ostream* _out) : TimerList(_out)
{
  writeHtmlHeader(out); 

  write(out, "<ul>");
}

HtmlTimerList::~HtmlTimerList()
{
  write(out, "</ul>");
  write(out, "</body></html>");
}

void HtmlTimerList::addTimer(cTimer* timer)
{
  write(out, "<li>");
  write(out, (char*)timer->File()); //TODO: add date, time and duration
  write(out, "\n");
}

JsonTimerList::JsonTimerList(std::ostream* _out) : TimerList(_out)
{

}

JsonTimerList::~JsonTimerList()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serTimers, "timers");
  serializer.finish();
}

void JsonTimerList::addTimer(cTimer* timer)
{
    SerTimer serTimer;
    serTimer.Start = timer->Start();
    serTimer.Stop = timer->Stop();
    serTimer.Priority = timer->Priority();
    serTimer.Lifetime = timer->Lifetime();
    serTimer.EventID = timer->Event() != NULL ? timer->Event()->EventID() : -1;
    serTimer.WeekDays = timer->WeekDays();
    serTimer.Day = timer->Day();
    serTimer.Channel = timer->Channel()->Number();
    serTimer.IsRecording = timer->Recording();
    serTimer.IsPending = timer->Pending();
    serTimer.FileName = UTF8Decode(timer->File());
    serTimers.push_back(serTimer);
}
