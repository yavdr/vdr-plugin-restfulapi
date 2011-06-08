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
  } else if ( request.method() == "PUT" ) {
     updateTimer(out, request, reply);
  } else {
    reply.httpReturn(501, "Only GET, DELETE, POST and PUT methods are supported.");
  }
}

void TimersResponder::updateTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  reply.httpReturn(403, "Updating timers currently not supported. Will be added in the future.");
}

void TimersResponder::createTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( Timers.BeingEdited() ) {
     reply.httpReturn(502, "Timers are being edited - try again later");
     return;
  }

  cxxtools::QueryParams q;
  q.parse_url(request.bodyStr());

  //std::string timer_id_str = q.param("timer_id"); only required to update timers

  int error = false;
  std::string error_message = "";
  static TimerValues v;

  int flags = v.ConvertFlags(q.param("flags"));
  std::string aux = v.ConvertAux(q.param("aux"));
  std::string file = v.ConvertFile(q.param("file"));
  int lifetime = v.ConvertLifetime(q.param("lifetime"));
  int priority = v.ConvertPriority(q.param("priority"));
  int stop = v.ConvertStop(q.param("stop"));
  int start = v.ConvertStart(q.param("start"));
  std::string weekdays = q.param("weekdays");
  int day = v.ConvertDay(q.param("day"));
  cChannel* channel = v.ConvertChannel(q.param("channel"));
  std::string event_id = q.param("event_id");
  cEvent* event = v.ConvertEvent(event_id, channel);

  if ( !v.IsFlagsValid(flags) ) { error = true; error_message += " Flags aren't valid!"; }
  if ( event_id.length() >0 && event == NULL ) { error = true; error_message += " EventID isn't valid!"; }
  if ( !v.IsFileValid(file) ) { error = true; error_message += " File isn't valid!"; }
  if ( !v.IsLifetimeValid(lifetime) ) { error = true; error_message += " Lifetime isn't valid!"; }
  if ( !v.IsPriorityValid(priority) ) { error = true; error_message += " Priority isn't valid!"; }
  if ( !v.IsStopValid(stop) ) { error = true; error_message += " Stop time isn't valid!"; }
  if ( !v.IsStartValid(start) ) { error = true; error_message += " Start time isn't valid!"; }
  if ( !v.IsWeekdaysValid(weekdays) ) { error = true; error_message += " Weekdays isn't valid!"; }
  if ( day <= (time(NULL)-(24*3600)) ) { error = true; error_message += " Day isn't valid!"; }
  if ( channel == NULL ) { error = true; error_message += " Channel isn't valid!"; }

  if (error) {
     reply.httpReturn(403, error_message);
     return;
  }

  // create timer
/*

  ostringstream builder;
  builder << flags_str << ":"
          << channelInstance->GetChannelID();
	  << ( weekdays_str != "-------" ? weekdays_str : "" )
          << ( weekdays_str == "-------" || day_str.empty() ? "" : "@" ) << day << ":"
          << start ":"
          << stop << ":"
          << priority << ":"
          << lifetime << ":"
          << replace(file_str, ":", "|") << ":" 
          << replace(aux_str, ":", "|");*/



  /*if ( timer_id != -1 ) {
     // update timer information
     cTimer* timer = Timers.Get(timer_id);
     // change the values here
     Timers.SetModified();
  } else if (channelInstance != NULL) {
     cString buffer = cString::sprintf("%u:%s:%s:%04d:%04d:%d:%d:%s:%s\n", flags, (const char*)channelInstance->GetChannelID().ToString(), *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file_str.c_str(), aux_str.c_str());
     esyslog("restfulapi:%u:%s:%s:%04d:%04d:%d:%d:%s:%s\n", flags, (const char*)channelInstance->GetChannelID().ToString(), *cTimer::PrintDay(day, weekdays, true), start, stop, priority, lifetime, file_str.c_str(), aux_str.c_str());
     cTimer *timer = new cTimer();
     if (timer->Parse(buffer)) {
        esyslog("restfulapi: successfully parsed");
        //add event id
        cTimer *t = Timers.GetTimer(timer);
        if ( !t ) {
           Timers.Add(timer);
           Timers.SetModified();
           esyslog("restfulapi: successfully added timer");
        }
     }
  }*/
}

void TimersResponder::deleteTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( Timers.BeingEdited() ) {
     reply.httpReturn(502, "Timers are being edited - try again later");
     return;
  }

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
 
  Timers.SetModified();

  if ( isFormat(params, ".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     timerList = (TimerList*)new JsonTimerList(&out);
  } else if ( isFormat(params, ".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     timerList = (TimerList*)new HtmlTimerList(&out);
  } else if ( isFormat(params, ".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     timerList = (TimerList*)new XmlTimerList(&out);
  } else {
     reply.httpReturn(404, "Resources are not available for the selected format. (Use: .json, .html or .xml)");
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
  si.addMember("is_active") <<= t.IsActive;
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
  si.getMember("is_active") >>= t.IsActive;
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
    serTimer.IsActive = timer->Flags() & 0x01 == 0x01 ? true : false;
    serTimers.push_back(serTimer);
}

void JsonTimerList::finish()
{
  cxxtools::JsonSerializer serializer(*out);
  serializer.serialize(serTimers, "timers");
  serializer.finish();
}

void XmlTimerList::init()
{
  write(out, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
  write(out, "<timers xmlns=\"http://www.domain.org/restfulapi/2011/timers-xml\">\n");
}

void XmlTimerList::addTimer(cTimer* timer)
{
  write(out, " <timer>\n");
  write(out, (const char*)cString::sprintf("  <param name=\"start\">%i</param>\n", timer->Start()) );
  write(out, (const char*)cString::sprintf("  <param name=\"stop\">%i</param>\n", timer->Stop()) );
  write(out, (const char*)cString::sprintf("  <param name=\"priority\">%i</param>\n", timer->Priority()) );
  write(out, (const char*)cString::sprintf("  <param name=\"lifetime\">%i</param>\n", timer->Lifetime()) );
  write(out, (const char*)cString::sprintf("  <param name=\"event_id\">%i</param>\n", timer->Event() != NULL ? timer->Event()->EventID() : -1) );
  write(out, (const char*)cString::sprintf("  <param name=\"weekdays\">%i</param>\n", timer->WeekDays()) );
  write(out, (const char*)cString::sprintf("  <param name=\"day\">%i</param>\n", (int)timer->Day()));
  write(out, (const char*)cString::sprintf("  <param name=\"channel\">%i</param>\n", timer->Channel()->Number()) );
  write(out, (const char*)cString::sprintf("  <param name=\"is_recording\">%s</param>\n", timer->Recording() ? "true" : "false" ) );
  write(out, (const char*)cString::sprintf("  <param name=\"is_pending\">%s</param>\n", timer->Pending() ? "true" : "false" ));
  write(out, (const char*)cString::sprintf("  <param name=\"filename\">%s</param>\n", encodeToXml(timer->File()).c_str()) );
  write(out, (const char*)cString::sprintf("  <param name=\"channelname\">%s</param>\n", encodeToXml(timer->Channel()->Name()).c_str()));
  write(out, (const char*)cString::sprintf("  <param name=\"is_active\">%s</param>\n", timer->Flags() & 0x01 == 0x01 ? "true" : "false" ));
  write(out, " </timer>\n");
}

void XmlTimerList::finish()
{
  write(out, "</timers>");
}

// --- TimerValues class ------------------------------------------------------------

int TimerValues::ConvertNumber(std::string v)
{
  static cxxtools::Regex numberregex("[0-9]*");
  if (!numberregex.match(v)) return -1;
  return atoi(v.c_str());
}

bool TimerValues::IsFlagsValid(int v)
{
  if ( v == 0x0000 || v == 0x0001 || v == 0x0002 || v == 0x0004 || v == 0x0008 || v == 0xFFFF ) 
     return true;
  return false;
}

bool TimerValues::IsFileValid(std::string v) 
{
  if ( v.length() > 0 && v.length() <= 40 ) 
     return true;
  return false;
}

bool TimerValues::IsLifetimeValid(int v) 
{
  if ( v >= 0 && v <= 99 )
     return true;
  return false;
}

bool TimerValues::IsPriorityValid(int v)
{
  return IsLifetimeValid(v); //uses the same values as the lifetime
}

bool TimerValues::IsStopValid(int v)
{
  int minutes = v % 100;
  int hours = (v - minutes) / 100;
  if ( minutes >= 0 && minutes < 60 && hours >= 0 && hours < 24 )
     return true;
  return false;

}

bool TimerValues::IsStartValid(int v)
{
  return IsStopValid(v); //uses the syntax as start time, f.e. 2230 means half past ten in the evening
}

bool TimerValues::IsWeekdaysValid(std::string v)
{
  static cxxtools::Regex regex("[\\-M][\\-T][\\-W][\\-T][\\-F][\\-S][\\-S]");
  return regex.match(v);
}

int TimerValues::ConvertFlags(std::string v)
{
  return ConvertNumber(v);
}

cEvent* TimerValues::ConvertEvent(std::string event_id, cChannel* channel)
{
  if ( channel == NULL ) return NULL;

  int eventid = ConvertNumber(event_id);
  if ( eventid == -1 ) return NULL;

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);
  if ( !Schedules ) return NULL;

  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());

  if ( !Schedule ) return NULL;

  return (cEvent*)Schedule->GetEvent(eventid);
}

cTimer* TimerValues::ConvertTimer(std::string v)
{
  int timer_id = ConvertNumber(v);
  return Timers.Get(timer_id);
}

std::string TimerValues::ConvertFile(std::string v)
{
  return replace(v, ":", "|");
}

std::string TimerValues::ConvertAux(std::string v)
{
  return ConvertFile(v);
}

int TimerValues::ConvertLifetime(std::string v)
{
  return ConvertNumber(v);
}

int TimerValues::ConvertPriority(std::string v)
{
  return ConvertNumber(v);
}

int TimerValues::ConvertStop(std::string v)
{
  return ConvertNumber(v);
}

int TimerValues::ConvertStart(std::string v)
{
  return ConvertNumber(v);
}

int TimerValues::ConvertDay(std::string v)
{
  return ConvertNumber(v);
}

cChannel* TimerValues::ConvertChannel(std::string v)
{
  int c = ConvertNumber(v);
  return getChannel(c);
}
