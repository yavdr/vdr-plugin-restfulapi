#include "timers.h"
using namespace std;

void TimersResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);
  if ( request.method() == "GET" ) {
     showTimers(out, request, reply);
  } else if ( request.method() == "DELETE" ) {
     deleteTimer(out, request, reply);
  } else if ( request.method() == "POST" ) {
     createOrUpdateTimer(out, request, reply, false);
  } else if ( request.method() == "PUT" ) {
     createOrUpdateTimer(out, request, reply, true);
  } else if (request.method() == "OPTIONS") {
     return;		 
  } else {
     reply.httpReturn(501, "Only GET, DELETE, POST and PUT methods are supported.");
  }
}

void TimersResponder::createOrUpdateTimer(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, bool update)
{
  QueryHandler q("/timers", request);

  if ( Timers.BeingEdited() ) {
     reply.httpReturn(502, "Timers are being edited - try again later");
     return;
  }

  int error = false;
  string error_values = "";
  static TimerValues v;

  int flags = v.ConvertFlags(q.getBodyAsString("flags"));
  string aux = v.ConvertAux(q.getBodyAsString("aux"));
  string file = v.ConvertFile(q.getBodyAsString("file"));
  int lifetime = v.ConvertLifetime(q.getBodyAsString("lifetime"));
  int priority = v.ConvertPriority(q.getBodyAsString("priority"));
  int stop = v.ConvertStop(q.getBodyAsString("stop"));
  int start = v.ConvertStart(q.getBodyAsString("start"));
  string weekdays = q.getBodyAsString("weekdays");
  string day = v.ConvertDay(q.getBodyAsString("day"));
  cChannel* chan = v.ConvertChannel(q.getBodyAsString("channel"));
  cTimer* timer_orig = v.ConvertTimer(q.getBodyAsString("timer_id"));
  
  if ( update == false ) { //create
     int eventid = q.getBodyAsInt("eventid");
     int minpre = q.getBodyAsInt("minpre");
     int minpost = q.getBodyAsInt("minpost");
     if (eventid >= 0 && chan != NULL) {
        cEvent* event = VdrExtension::GetEventById((tEventID)eventid, chan);
      
        if (event == NULL) {
           reply.httpReturn(407, "eventid invalid");
           return;
        } else {
           if (minpre < 0) minpre = 0;
           if (minpost < 0) minpost = 0;
           if (!v.IsFlagsValid(flags)) flags = 1;
           if (!v.IsFileValid(file)) file = (string)event->Title();
           if (!v.IsWeekdaysValid(weekdays)) weekdays = "-------";
           if (!v.IsLifetimeValid(lifetime)) lifetime = 50;
           if (!v.IsPriorityValid(priority)) priority = 99;
           chan = VdrExtension::getChannel((const char*)event->ChannelID().ToString());
           if (!v.IsStartValid(start) || !v.IsStopValid(stop) || !v.IsDayValid(day)) {
              time_t estart = event->StartTime();
              time_t estop = event->EndTime();
              struct tm *starttime = localtime(&estart);
            
              ostringstream daystream;
              daystream << StringExtension::addZeros((starttime->tm_year + 1900), 4) << "-"
                        << StringExtension::addZeros((starttime->tm_mon + 1), 2) << "-"
                        << StringExtension::addZeros((starttime->tm_mday), 2);
              day = daystream.str();
 
              start = starttime->tm_hour * 100 + starttime->tm_min - ((int)(minpre/60))*100 - minpre%60;

              struct tm *stoptime = localtime(&estop);
              stop = stoptime->tm_hour * 100 + stoptime->tm_min + ((int)(minpost/60))*100 + minpost%60;
           }
        }
     } else {
        if ( !v.IsFlagsValid(flags) ) { flags = 1; }
        if ( !v.IsFileValid(file) ) { error = true; error_values += "file, "; }
        if ( !v.IsLifetimeValid(lifetime) ) { lifetime = 50; }
        if ( !v.IsPriorityValid(priority) ) { priority = 99; }
        if ( !v.IsStopValid(stop) ) { error = true; error_values += "stop, "; }
        if ( !v.IsStartValid(start) ) { error = true; error_values += "start, "; }
        if ( !v.IsWeekdaysValid(weekdays) ) { error = true; error_values += "weekdays, "; }
        if ( !v.IsDayValid(day)&& !day.empty() ) { error = true; error_values += "day, "; }
        if ( chan == NULL ) { error = true; error_values += "channel, "; }
     }
  } else { //update
     if ( timer_orig == NULL ) { error = true; error_values += "timer_id, "; }
     if ( !error ) {
        if ( !v.IsFlagsValid(flags) ) { flags = timer_orig->Flags(); }
        if ( !v.IsFileValid(file) ) { file = (string)timer_orig->File(); }
        if ( !v.IsLifetimeValid(lifetime) ) { lifetime = timer_orig->Lifetime(); }
        if ( !v.IsPriorityValid(priority) ) { priority = timer_orig->Priority(); }
        if ( !v.IsStopValid(stop) ) { stop = timer_orig->Stop(); }
        if ( !v.IsStartValid(start) ) { start = timer_orig->Start(); }
        if ( !v.IsWeekdaysValid(weekdays) ) { weekdays = v.ConvertWeekdays(timer_orig->WeekDays()); }
        if ( !v.IsDayValid(day) ) { day = v.ConvertDay(timer_orig->Day()); }
        if ( chan == NULL ) { chan = (cChannel*)timer_orig->Channel(); }
     }
  }

  if (error) {
     string error_message = (string)"The following parameters aren't valid: " + error_values.substr(0, error_values.length()-2) + (string)"!";
     reply.httpReturn(403, error_message);
     return;
  }
 
  ostringstream builder;
  builder << flags << ":"
          << (const char*)chan->GetChannelID().ToString() << ":"
	  << ( weekdays != "-------" ? weekdays : "" )
          << ( weekdays == "-------" || day.empty() ? "" : "@" ) << day << ":"
          << start << ":"
          << stop << ":"
          << priority << ":"
          << lifetime << ":"
          << file << ":" 
          << aux;

  dsyslog("restfulapi: /%s/ ", builder.str().c_str());
  chan = NULL;
  if ( update == false ) { // create timer
     cTimer* timer = new cTimer();
     if ( timer->Parse(builder.str().c_str()) ) { 
        cTimer* checkTimer = Timers.GetTimer(timer);
        if ( checkTimer != NULL ) {
           delete timer;
           reply.httpReturn(403, "Timer already defined!"); 
           esyslog("restfulapi: Timer already defined!");
        } else {
           replyCreatedId(timer, request, reply, out);
           timer->SetEventFromSchedule();
           Timers.Add(timer);
           Timers.SetModified();
           esyslog("restfulapi: timer created!");
        }
     } else {
        reply.httpReturn(403, "Creating timer failed!");
        esyslog("restfulapi: timer creation failed!");
     }
  } else {
     if ( timer_orig->Parse(builder.str().c_str()) ) {
        timer_orig->SetEventFromSchedule();
        Timers.SetModified();
        replyCreatedId(timer_orig, request, reply, out);
        esyslog("restfulapi: updating timer successful!");
     } else { 
        reply.httpReturn(403, "updating timer failed!");
        esyslog("restfulapi: updating timer failed!");
     }
  }
}

void TimersResponder::replyCreatedId(cTimer* timer, cxxtools::http::Request& request, cxxtools::http::Reply& reply, ostream& out)
{
  QueryHandler q("/timers", request);
  TimerList* timerList;

  if ( q.isFormat(".html") ) {
     reply.addHeader("Content-Type", "text/html; charset=utf-8");
     timerList = (TimerList*)new HtmlTimerList(&out);
  } else if ( q.isFormat(".xml") ) {
     reply.addHeader("Content-Type", "text/xml; charset=utf-8");
     timerList = (TimerList*)new XmlTimerList(&out);
  } else {
     reply.addHeader("Content-Type", "application/json; charset=utf-8");
     timerList = (TimerList*)new JsonTimerList(&out);
  }

  timerList->init();
  timerList->addTimer(timer);
  timerList->setTotal(1);
  timerList->finish();
  delete timerList;
}

void TimersResponder::deleteTimer(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/timers", request);

  if ( Timers.BeingEdited() ) {
     reply.httpReturn(502, "Timers are being edited - try again later");
     return;
  }

  TimerValues v;

  cTimer* timer = v.ConvertTimer(q.getParamAsString(0));
 
  if ( timer == NULL) {
     reply.httpReturn(404, "Timer id invalid!");
  } else {
     if ( timer->Recording() ) {
        timer->Skip();
        cRecordControls::Process(time(NULL));
     }
     Timers.Del(timer);
     Timers.SetModified();
     reply.httpReturn(200, "Timer deleted."); 
  }
}

void TimersResponder::showTimers(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
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

  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");

  string timer_id = q.getParamAsString(0);

  if ( start_filter >= 0 && limit_filter >= 1 ) {
     timerList->activateLimit(start_filter, limit_filter);
  }

  timerList->init();

  vector< cTimer* > timers = VdrExtension::SortedTimers();
  for (int i=0;i<(int)timers.size();i++)
  {
     if ( VdrExtension::getTimerID(timers[i]) == timer_id || timer_id.length() == 0 ) {
        timerList->addTimer(timers[i]);   
     }
  }
  timerList->setTotal((int)timers.size());

  timerList->finish();
  delete timerList;   
}

void operator<<= (cxxtools::SerializationInfo& si, const SerTimer& t)
{
  si.addMember("id") <<= t.Id;
  si.addMember("flags") <<= t.Flags;
  si.addMember("start") <<= t.Start;
  si.addMember("start_timestamp") <<= t.StartTimeStamp;
  si.addMember("stop_timestamp") <<= t.StopTimeStamp;
  si.addMember("stop") <<= t.Stop;
  si.addMember("priority") <<= t.Priority;
  si.addMember("lifetime") <<= t.Lifetime;
  si.addMember("event_id") <<= t.EventID;
  si.addMember("weekdays") <<= t.WeekDays;
  si.addMember("day") <<= t.Day;
  si.addMember("channel") <<= t.Channel;
  si.addMember("filename") <<= t.FileName;
  si.addMember("channel_name") <<= t.ChannelName;
  si.addMember("is_pending") <<= t.IsPending;
  si.addMember("is_recording") <<= t.IsRecording;
  si.addMember("is_active") <<= t.IsActive;
}

TimerList::TimerList(ostream *out)
{
  s = new StreamExtension(out);
}

TimerList::~TimerList()
{
  delete s;
}

void HtmlTimerList::init()
{
  s->writeHtmlHeader("HtmlTimerList");
  s->write("<ul>");
}

void HtmlTimerList::addTimer(cTimer* timer)
{
  if ( filtered() ) return;
  s->write("<li>");
  s->write((char*)timer->File()); //TODO: add date, time and duration
  s->write("\n");
}

void HtmlTimerList::finish()
{
  s->write("</ul>");
  s->write("</body></html>");
}

void JsonTimerList::addTimer(cTimer* timer)
{
  if ( filtered() ) return;
  static TimerValues v;

  SerTimer serTimer;
  serTimer.Id = StringExtension::UTF8Decode(VdrExtension::getTimerID(timer));
  serTimer.Flags = timer->Flags();
  serTimer.Start = timer->Start();
  serTimer.Stop = timer->Stop();
  serTimer.Priority = timer->Priority();
  serTimer.Lifetime = timer->Lifetime();
  serTimer.EventID = timer->Event() != NULL ? timer->Event()->EventID() : -1;
  serTimer.WeekDays = StringExtension::UTF8Decode(v.ConvertWeekdays(timer->WeekDays()));
  serTimer.Day = StringExtension::UTF8Decode(v.ConvertDay(timer->Day()));
  serTimer.Channel = StringExtension::UTF8Decode((const char*)timer->Channel()->GetChannelID().ToString());
  serTimer.IsRecording = timer->Recording();
  serTimer.IsPending = timer->Pending();
  serTimer.FileName = StringExtension::UTF8Decode(timer->File());
  serTimer.ChannelName = StringExtension::UTF8Decode(timer->Channel()->Name());
  serTimer.IsActive = timer->Flags() & 0x01 == 0x01 ? true : false;

  int tstart = timer->Day() - ( timer->Day() % 3600 ) + ((int)(timer->Start()/100)) * 3600 + ((int)(timer->Start()%100)) * 60;
  int tstop = timer->Day() - ( timer->Day() % 3600 ) + ((int)(timer->Stop()/100)) * 3600 + ((int)(timer->Stop()%100)) * 60;

  //if a timer starts before and ends after midnight, add a day to tstop
  if ( (int)(timer->Start()) > (int)(timer->Stop()) )
    tstop += 86400;

  serTimer.StartTimeStamp = StringExtension::UTF8Decode(StringExtension::dateToString((time_t)tstart));
  serTimer.StopTimeStamp = StringExtension::UTF8Decode(StringExtension::dateToString((time_t)tstop));

  serTimers.push_back(serTimer);
}

void JsonTimerList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(serTimers, "timers");
  serializer.serialize(serTimers.size(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void XmlTimerList::init()
{
  counter = 0;
  s->writeXmlHeader();
  s->write("<timers xmlns=\"http://www.domain.org/restfulapi/2011/timers-xml\">\n");
}

void XmlTimerList::addTimer(cTimer* timer)
{
  if ( filtered() ) return;
  static TimerValues v;

  s->write(" <timer>\n");
  s->write(cString::sprintf("  <param name=\"id\">%s</param>\n", StringExtension::encodeToXml(VdrExtension::getTimerID(timer)).c_str()));
  s->write(cString::sprintf("  <param name=\"flags\">%i</param>\n", timer->Flags()));
  s->write(cString::sprintf("  <param name=\"start\">%i</param>\n", timer->Start()) );
  s->write(cString::sprintf("  <param name=\"stop\">%i</param>\n", timer->Stop()) );

  int tstart = timer->Day() - ( timer->Day() % 3600 ) + ((int)(timer->Start()/100)) * 3600 + ((int)(timer->Start()%100)) * 60;
  int tstop = timer->Day() - ( timer->Day() % 3600 ) + ((int)(timer->Stop()/100)) * 3600 + ((int)(timer->Stop()%100)) * 60;

  s->write(cString::sprintf("  <param name=\"start_timestamp\">%s</param>\n", StringExtension::encodeToXml(StringExtension::dateToString(tstart)).c_str()));
  s->write(cString::sprintf("  <param name=\"stop_timestamp\">%s</param>\n", StringExtension::encodeToXml(StringExtension::dateToString(tstop)).c_str()));

  s->write(cString::sprintf("  <param name=\"priority\">%i</param>\n", timer->Priority()) );
  s->write(cString::sprintf("  <param name=\"lifetime\">%i</param>\n", timer->Lifetime()) );
  s->write(cString::sprintf("  <param name=\"event_id\">%i</param>\n", timer->Event() != NULL ? timer->Event()->EventID() : -1) );
  s->write(cString::sprintf("  <param name=\"weekdays\">%s</param>\n", StringExtension::encodeToXml(v.ConvertWeekdays(timer->WeekDays())).c_str()));
  s->write(cString::sprintf("  <param name=\"day\">%s</param>\n", StringExtension::encodeToXml(v.ConvertDay(timer->Day())).c_str()));
  s->write(cString::sprintf("  <param name=\"channel\">%s</param>\n", StringExtension::encodeToXml((const char*)timer->Channel()->GetChannelID().ToString()).c_str()) );
  s->write(cString::sprintf("  <param name=\"is_recording\">%s</param>\n", timer->Recording() ? "true" : "false" ) );
  s->write(cString::sprintf("  <param name=\"is_pending\">%s</param>\n", timer->Pending() ? "true" : "false" ));
  s->write(cString::sprintf("  <param name=\"file_name\">%s</param>\n", StringExtension::encodeToXml(timer->File()).c_str()) );
  s->write(cString::sprintf("  <param name=\"channel_name\">%s</param>\n", StringExtension::encodeToXml(timer->Channel()->Name()).c_str()));
  s->write(cString::sprintf("  <param name=\"is_active\">%s</param>\n", timer->Flags() & 0x01 == 0x01 ? "true" : "false" ));
  s->write(" </timer>\n");
}

void XmlTimerList::finish()
{
  s->write((const char*)cString::sprintf(" <count>%i</count><total>%i</total>", Count(), total));
  s->write("</timers>");
}

// --- TimerValues class ------------------------------------------------------------

queue<int> TimerValues::ConvertToBinary(int v)
{
   int b;
   queue <int> res;

   while ( v != 0) {
     b = v % 2;
     res.push(b);
     v = (v-b) / 2;
   }
   return res;
}

bool TimerValues::IsDayValid(string v)
{
  static cxxtools::Regex regex("[0-9]{4,4}-[0-9]{1,2}-[0-9]{1,2}");
  return regex.match(v);
}

bool TimerValues::IsFlagsValid(int v)
{
  if ( v == 0x0000 || v == 0x0001 || v == 0x0002 || v == 0x0004 || v == 0x0008 || v == 0xFFFF ) 
     return true;
  return false;
}

bool TimerValues::IsFileValid(string v) 
{
  if ( v.length() > 0 && v.length() <= 99 ) 
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

bool TimerValues::IsWeekdaysValid(string v)
{
  /*static cxxtools::Regex regex("[\\-M][\\-T][\\-W][\\-T][\\-F][\\-S][\\-S]");
  return regex.match(v);*/
  if ( v.length() != 7 ) return false;
  const char* va = v.c_str();
  if ( va[0] != '-' && va[0] != 'M' ) return false;
  if ( va[1] != '-' && va[1] != 'T' ) return false;
  if ( va[2] != '-' && va[2] != 'W' ) return false;
  if ( va[3] != '-' && va[3] != 'T' ) return false;
  if ( va[4] != '-' && va[4] != 'F' ) return false;
  if ( va[5] != '-' && va[5] != 'S' ) return false;
  if ( va[6] != '-' && va[6] != 'S' ) return false;
  return true;
}

int TimerValues::ConvertFlags(string v)
{
  return StringExtension::strtoi(v);
}

cEvent* TimerValues::ConvertEvent(string event_id, cChannel* channel)
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

string TimerValues::ConvertFile(string v)
{
  return StringExtension::replace(v, ":", "|");
}

string TimerValues::ConvertAux(string v)
{
  return ConvertFile(v);
}

int TimerValues::ConvertLifetime(string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertPriority(string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertStop(string v)
{
  return StringExtension::strtoi(v);
}

int TimerValues::ConvertStart(string v)
{
  return StringExtension::strtoi(v);
}

string TimerValues::ConvertDay(time_t v)
{
  if (v==0) return "";
  struct tm *timeinfo = localtime(&v); //must not be deleted because it's statically allocated by localtime
  ostringstream str;
  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1; //append 0, vdr wants two digits!
  int day = timeinfo->tm_mday; //append 0, vdr wants two digits!
  str << year << "-" 
      << (month < 10 ? "0" : "") << month << "-" 
      << (day < 10 ? "0" : "") << day;
  return str.str();
}

string TimerValues::ConvertDay(string v)
{
  if ( !IsDayValid(v) ) return "wrong format";
  //now append 0 (required by vdr) if month/day don't already have two digits
  int a = v.find_first_of('-');
  int b = v.find_last_of('-');

  string year = v.substr(0, a);
  string month = v.substr(a+1, b-a-1);
  string day = v.substr(b+1);

  ostringstream res;
  res << year << "-"
      << (month.length() == 1 ? "0" : "") << month << "-"
      << (day.length() == 1 ? "0" : "") << day;
  return res.str();
}

cChannel* TimerValues::ConvertChannel(string v)
{
  return VdrExtension::getChannel(v);
}

cTimer* TimerValues::ConvertTimer(string v)
{
  return VdrExtension::getTimer(v);
}

string TimerValues::ConvertWeekdays(int v)
{
  queue<int> b = ConvertToBinary(v);
  int counter = 0;
  ostringstream res;
  while ( !b.empty() && counter < 7 ) {
     int val = b.front();
     switch(counter) {
       case 0: res << (val == 1 ? 'M' : '-'); break;
       case 1: res << (val == 1 ? 'T' : '-'); break;
       case 2: res << (val == 1 ? 'W' : '-'); break;
       case 3: res << (val == 1 ? 'T' : '-'); break;
       case 4: res << (val == 1 ? 'F' : '-'); break;
       case 5: res << (val == 1 ? 'S' : '-'); break;
       case 6: res << (val == 1 ? 'S' : '-'); break;
     }
     b.pop();
     counter++;
  }
  while ( counter < 7 ) {
     res << '-';
     counter++;
  }
  return res.str();
}

int TimerValues::ConvertWeekdays(string v)
{
  const char* str = v.c_str();
  int res = 0;
  if ( str[0] == 'M' ) res += 64;
  if ( str[1] == 'T' ) res += 32;
  if ( str[2] == 'W' ) res += 16;
  if ( str[3] == 'T' ) res += 8;
  if ( str[4] == 'F' ) res += 4;
  if ( str[5] == 'S' ) res += 2;
  if ( str[6] == 'S' ) res += 1;
  return res;
}
