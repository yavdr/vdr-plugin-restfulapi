#include "timers.h"

void TimersResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
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

  QueryHandler q("/timers", request);

  //std::string timer_id_str = q.param("timer_id"); only required to update timers

  int error = false;
  std::string error_values = "";
  static TimerValues v;

  int flags = v.ConvertFlags(q.getOptionAsString("flags"));
  std::string aux = v.ConvertAux(q.getOptionAsString("aux"));
  std::string file = v.ConvertFile(q.getOptionAsString("file"));
  int lifetime = v.ConvertLifetime(q.getOptionAsString("lifetime"));
  int priority = v.ConvertPriority(q.getOptionAsString("priority"));
  int stop = v.ConvertStop(q.getOptionAsString("stop"));
  int start = v.ConvertStart(q.getOptionAsString("start"));
  std::string weekdays = q.getOptionAsString("weekdays");
  int day = v.ConvertDay(q.getOptionAsString("day"));
  cChannel* chan = v.ConvertChannel(q.getOptionAsString("channel"));
  std::string event_id = q.getOptionAsString("event_id");
  cEvent* event = v.ConvertEvent(event_id, chan);

  if ( !v.IsFlagsValid(flags) ) { error = true; error_values += "flags, "; }
  if ( event_id.length() > 0 && event == NULL ) { error = true; error_values += "event_id, "; }
  if ( !v.IsFileValid(file) ) { error = true; error_values += "file, "; }
  if ( !v.IsLifetimeValid(lifetime) ) { error = true; error_values += "lifetime, "; }
  if ( !v.IsPriorityValid(priority) ) { error = true; error_values += "priority, "; }
  if ( !v.IsStopValid(stop) ) { error = true; error_values += "stop, "; }
  if ( !v.IsStartValid(start) ) { error = true; error_values += "start, "; }
  if ( !v.IsWeekdaysValid(weekdays) ) { error = true; error_values += "weekdays, "; }
  //if ( day <= (time(NULL)-(24*3600)) ) { error = true; error_values += "day, "; }
  if ( chan == NULL ) { error = true; error_values += "channel, "; }

  if (error) {
     std::string error_message = (std::string)"The following parameters aren't valid: " + error_values.substr(0, error_values.length()-2) + (std::string)"!";
     reply.httpReturn(403, error_message);
     return;
  }


  std::ostringstream builder;
  builder << flags << ":"
          << (const char*)chan->GetChannelID().ToString()
	  << ( weekdays != "-------" ? weekdays : "" )
          << ( weekdays == "-------" || day == -1 ? "" : "@" ) << day << ":"
          << start << ":"
          << stop << ":"
          << priority << ":"
          << lifetime << ":"
          << file << ":" 
          << aux;

  esyslog("restfulapi: /%s/ ", builder.str().c_str());
  chan = NULL;


  /*cTimer* timer = new cTimer();
  if ( timer->Parse(builder.str().c_str()) ) { 
     Timers.Add(timer);
     Timers.SetModified();
  }*/
}

void TimersResponder::deleteTimer(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( Timers.BeingEdited() ) {
     reply.httpReturn(502, "Timers are being edited - try again later");
     return;
  }

  QueryHandler q("/timers", request);

  int timer_number = q.getParamAsInt(0);
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
  QueryHandler q("/timers", request);
  TimerList* timerList;
 
  Timers.SetModified();

  if ( q.isFormat(".json") ) {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     timerList = (TimerList*)new JsonTimerList(&out);
  } else if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     timerList = (TimerList*)new HtmlTimerList(&out);
  } else if ( q.isFormat(".xml") ) {
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
    serTimer.FileName = StringExtension::UTF8Decode(timer->File());
    serTimer.ChannelName = StringExtension::UTF8Decode(timer->Channel()->Name());
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
  write(out, (const char*)cString::sprintf("  <param name=\"filename\">%s</param>\n", StringExtension::encodeToXml(timer->File()).c_str()) );
  write(out, (const char*)cString::sprintf("  <param name=\"channelname\">%s</param>\n", StringExtension::encodeToXml(timer->Channel()->Name()).c_str()));
  write(out, (const char*)cString::sprintf("  <param name=\"is_active\">%s</param>\n", timer->Flags() & 0x01 == 0x01 ? "true" : "false" ));
  write(out, " </timer>\n");
}

void XmlTimerList::finish()
{
  write(out, "</timers>");
}

// --- TimerValues class ------------------------------------------------------------

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
  return StringExtension::strtoi(v);
}

cEvent* TimerValues::ConvertEvent(std::string event_id, cChannel* channel)
{
  if ( channel == NULL ) return NULL;

  int eventid = StringExtension::strtoi(event_id);
  if ( eventid <= -1 ) return NULL;

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);
  if ( !Schedules ) return NULL;

  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());

  if ( !Schedule ) return NULL;

  return (cEvent*)Schedule->GetEvent(eventid);
}

cTimer* TimerValues::ConvertTimer(std::string v)
{
  int timer_id = StringExtension::strtoi(v);
  return Timers.Get(timer_id);
}

std::string TimerValues::ConvertFile(std::string v)
{
  return StringExtension::replace(v, ":", "|");
}

std::string TimerValues::ConvertAux(std::string v)
{
  return ConvertFile(v);
}

int TimerValues::ConvertLifetime(std::string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertPriority(std::string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertStop(std::string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertStart(std::string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertDay(std::string v)
{
  return StringExtension::strtoi(v);
}

cChannel* TimerValues::ConvertChannel(std::string v)
{
  int c = StringExtension::strtoi(v);
  return VdrExtension::getChannel(c);
}
