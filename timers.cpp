#include "timers.h"

void TimersResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/timers", request.url());

  if ( request.method() == "GET" ) {
     showTimers(out, request, reply);
  } else if ( request.method() == "DELETE" ) {
     deleteTimer(out, request, reply);
  } else if ( request.method() == "POST" ) {
     createTimer(out, request, reply);
  } else {
    reply.httpReturn(501, "Only GET, DELETE and POST methods are supported.");
  }
}

void TimersResponder::createTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  reply.httpReturn(501, "Creating timer is not implemented yet.");
}

void TimersResponder::deleteTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/timers", request.url());
  int timer_number = getIntParam(params, 0);
  timer_number--; //first timer ist 0 and not 1

  int timer_count = Timers.Count();

  if ( timer_number < 0 || timer_number >= timer_count) {
     reply.httpReturn(404, "Timer number invalid!");
  } else {
     cTimer* timer = Timers.Get(timer_number);
     if ( timer ) {
        if ( !Timers.BeingEdited())
        {
           if ( timer->Recording() ) {
              timer->Skip();
              cRecordControls::Process(time(NULL));
           }
           Timers.Del(timer);
           Timers.SetModified();
        }
     }
  }
}

void TimersResponder::showTimers(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string params = getRestParams((std::string)"/timers", request.url());
  TimerList* timerList;

  if ( isFormat(params, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     timerList = (TimerList*)new JsonTimerList(&out);
  } else if ( isFormat(params, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     timerList = (TimerList*)new HtmlTimerList(&out);
  } else {
     reply.httpReturn(404, "Resources are not available for the selected format. (Use: .json or .html)");
     return;
  }

  timerList->init();

  int timer_count = Timers.Count();
  cTimer *timer;
  for (int i=0;i<timer_count;i++)
  {
     timer = Timers.Get(i);
     timerList->addTimer(timer);   
  }

  timerList->finish();
  delete timerList;   
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
  si.addMember("channelname") <<= t.ChannelName;
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
  si.getMember("channel_name") >>= t.ChannelName;
  si.getMember("is_pending") >>= t.IsPending;
  si.getMember("is_recording") >>= t.IsRecording;
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTimers& t)
{
  si.addMember("rows") <<= t.timer;
}

void HtmlTimerList::init()
{
  writeHtmlHeader(out); 

  write(out, "<ul>");
}

void HtmlTimerList::addTimer(cTimer* timer)
{
  write(out, "<li>");
  write(out, (char*)timer->File()); //TODO: add date, time and duration
  write(out, "\n");
}

void HtmlTimerList::finish()
{
  write(out, "</ul>");
  write(out, "</body></html>");
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
    serTimer.ChannelName = UTF8Decode(timer->Channel()->Name());
    serTimers.push_back(serTimer);
}

void JsonTimerList::finish()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serTimers, "timers");
  serializer.finish();
}
