/*
 * serverthread.cpp: JSONAPI plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "serverthread.h"
cxxtools::Utf8Codec codec;

// TimersResponder
//
class TimersResponder : public cxxtools::http::Responder
{
  public:
    explicit TimersResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
     virtual void reply(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

//implementation not final!!!
void TimersResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  esyslog("jsonapi, timers: /%s/", request.method().c_str());
 
  SerTimer serTimer;
  std::vector < struct SerTimer > serTimers;

  int timer_count = Timers.Count();
  cTimer *timer;
  for (int i=0;i<timer_count;i++)
  {
    timer = Timers.Get(i);
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
    serTimer.FileName = codec.decode(timer->File());
    serTimers.push_back(serTimer);
  }

  reply.addHeader("Content-Type", "application/json; charset=utf-8");
  cxxtools::JsonSerializer serializer(out);
  serializer.serialize(serTimers, "timers");
  
  serializer.finish();
}

//ChannelsService
//
typedef cxxtools::http::CachedService<TimersResponder> TimersService;

// ChannelsResponder
//
class ChannelsResponder : public cxxtools::http::Responder
{
  public:
    explicit ChannelsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

void ChannelsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  SerChannel serChannel;
  std::vector < struct SerChannel > serChannels;

  reply.addHeader("Content-Type", "application/json; charset=utf-8");
  cxxtools::JsonSerializer serializer(out);

  std::string suffix = (std::string) ".ts";  

  for(cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
    if(!channel->GroupSep()) {
      serChannel.Name = codec.decode(channel->Name());
      serChannel.Number = channel->Number();
      serChannel.Transponder = channel->Transponder();
      serChannel.Stream = codec.decode(((std::string)channel->GetChannelID().ToString() + (std::string)suffix).c_str());
      serChannel.IsAtsc = channel->IsAtsc();
      serChannel.IsCable = channel->IsCable();
      serChannel.IsSat = channel->IsSat();
      serChannel.IsTerr = channel->IsTerr();
      serChannels.push_back(serChannel);
    }
  }
  serializer.serialize(serChannels, "channels");
  serializer.finish();
}

// ChannelsService
//
typedef cxxtools::http::CachedService<ChannelsResponder> ChannelsService;

// EventResponder
// 
class EventsResponder : public cxxtools::http::Responder
{
  public:
    explicit EventsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

void EventsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  std::string qparams = request.qparams();
  
  int channel_number = GetIntParam(qparams, (std::string)"channel=");
  int from = GetIntParam(qparams, (std::string)"from=");
  int timespan = GetIntParam(qparams, (std::string)"timespan=");
  
  dsyslog("jsonapi: %s ///%i///%i///%i///", qparams.c_str(), channel_number, from, timespan);

  if ( channel_number == -1 || !channel_number ) return; 
  cChannel* channel = GetChannel(channel_number);
  if ( !channel ) return;

  if ( from == -1 ) from = time(NULL); // default time is now
  if ( timespan == -1 ) timespan = 3600; // default timespan is one hour
  
  int to = from + timespan;

  SerEvent serEvent;
  std::vector < struct SerEvent > serEvents;
  cxxtools::String eventTitle;
  cxxtools::String eventShortText;
  cxxtools::String eventDescription;
  cxxtools::String empty = codec.decode("");

  cSchedulesLock MutexLock;
  const cSchedules *Schedules = cSchedules::Schedules(MutexLock);

  if(!Schedules) return;

  const cSchedule *Schedule = Schedules->GetSchedule(channel->GetChannelID());
  
  if(!Schedule) return;


  for(const cEvent* event = Schedule->Events()->First(); event; event = Schedule->Events()->Next(event))
  {
    int ts = event->StartTime();
    int te = ts + event->Duration();
    if ( ts <= to && te >= from ) {
       if( !event->Title() ) { eventTitle = empty; } else { eventTitle = codec.decode(event->Title()); }
       if( !event->ShortText() ) { eventShortText = empty; } else { eventShortText = codec.decode(event->ShortText()); }
       if( !event->Description() ) { eventDescription = empty; } else { eventDescription = codec.decode(event->Description()); }

       serEvent.Id = event->EventID();
       serEvent.Title = eventTitle;
       serEvent.ShortText = eventShortText;
       serEvent.Description = eventDescription;
       serEvent.StartTime = event->StartTime();
       serEvent.Duration = event->Duration();
       serEvents.push_back(serEvent);
    }else{
      if(ts > to) break;
    }
  }
  
  reply.addHeader("Content-Type", "application/json; charset=utf-8");
  cxxtools::JsonSerializer serializer(out);
  serializer.serialize(serEvents, "events");
  serializer.finish();
}


// EventsService
//
typedef cxxtools::http::CachedService<EventsResponder> EventsService;

// RecordingsResponder
//
class RecordingsResponder : public cxxtools::http::Responder
{
  public:
    explicit RecordingsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }

    virtual void reply(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};


void RecordingsResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  SerRecording serRecording;
  std::vector < struct SerRecording > serRecordings;

  reply.addHeader("Content-Type", "application/json; charset=utf-8");
  cxxtools::JsonSerializer serializer(out);
  for (cRecording* recording = Recordings.First(); recording; recording = Recordings.Next(recording)) {
    serRecording.Name = codec.decode(recording->Name());
    serRecording.FileName = codec.decode(recording->FileName());
    serRecording.IsNew = recording->IsNew();
    serRecording.IsEdited = recording->IsEdited();
    serRecording.IsPesRecording = recording->IsPesRecording();
    serRecordings.push_back(serRecording);
  }
  serializer.serialize(serRecordings, "recordings");
  serializer.finish();
}


// RecordingService
//
typedef cxxtools::http::CachedService<RecordingsResponder> RecordingsService;

cServerThread::cServerThread ()
{
  active = false;

  listenIp = "0.0.0.0";
  listenPort = 8002;

  isyslog("create server");
  server = new cxxtools::http::Server(loop, listenIp, listenPort);

  Start ();
}

cServerThread::~cServerThread ()
{
  if (active)
  {
    active = false;
    Cancel (3);
  }
  delete server;
}

void cServerThread::Action(void)
{
  active = true;

  ChannelsService channelsService;
  EventsService eventsService;
  RecordingsService recordingsService;
  TimersService timersService;

  server->addService("/channels.json", channelsService);
  server->addService("/events.json", eventsService);
  server->addService("/recordings.json", recordingsService);
  server->addService("/timers.json", timersService);

  loop.run();

  dsyslog("JSONAPI: server thread ended (pid=%d)", getpid());
}
