/*
 * serverthread.cpp: JSONAPI plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "serverthread.h"

void cServerThread::Initialize()
{
  active = false; 

  listenIp = Settings::get()->Ip();
  listenPort = Settings::get()->Port();

  isyslog("create server");
  server = new cxxtools::http::Server(loop, listenIp, listenPort);
}

void cServerThread::Stop() {
  active = false;
  loop.exit();
  int now = time(NULL);
  esyslog("restfulapi: will end server thread: /%i/", now);
  usleep(100000);//100ms//sleep(1);
  delete server;
}

cServerThread::~cServerThread()
{
  Cancel(0);
}

void cServerThread::Action(void)
{
  active = true;

  AudioService audioService; 
  ChannelsService channelsService;
  EventsService eventsService;  
  InfoService infoService;
  OsdService osdService;  
  RecordingsService recordingsService;
  RemoteService remoteService;
  SearchTimersService searchTimersService;  
  TimersService timersService;
  
  RestfulServices* services = RestfulServices::get();
  
  RestfulService* audio = new RestfulService("/audio", true, 1);  
  RestfulService* channels = new RestfulService("/channels", true, 1);
  RestfulService* channelGroups = new RestfulService("/channels/groups", true, 1, channels);
  RestfulService* channelImage = new RestfulService("/channels/image", true, 1, channels);
  RestfulService* events = new RestfulService("/events", true, 1);
  RestfulService* eventsImage = new RestfulService("/events/image", true, 1, events);
  RestfulService* eventsSearch = new RestfulService("/events/search", false, 1, events);
  RestfulService* info = new RestfulService("/info", true, 1);  
  RestfulService* osd = new RestfulService("/osd", true, 1); 
  RestfulService* recordings = new RestfulService("/recordings", true, 1);
  RestfulService* recordingsCut = new RestfulService("/recordings/cut", true, 1, recordings);
  RestfulService* recordingsMarks = new RestfulService("/recordings/marks", true, 1, recordings);
  RestfulService* recordingsPlay = new RestfulService("/recordings/play", true, 1, recordings);
  RestfulService* recordingsRewind = new RestfulService("/recordings/rewind", true, 1, recordings);
  RestfulService* remote = new RestfulService("/remote", true, 1);
  RestfulService* searchtimers = new RestfulService("/searchtimers", false, 1);  
  RestfulService* timers = new RestfulService("/timers", true, 1);
  
  services->appendService(audio);  
  services->appendService(channels);
  services->appendService(channelGroups);
  services->appendService(channelImage);
  services->appendService(events);
  services->appendService(eventsImage);
  services->appendService(eventsSearch);
  services->appendService(info);  
  services->appendService(osd);  
  services->appendService(recordings);
  services->appendService(recordingsCut);
  services->appendService(recordingsMarks);
  services->appendService(recordingsPlay);
  services->appendService(recordingsRewind);
  services->appendService(remote);
  services->appendService(searchtimers);  
  services->appendService(timers);

  server->addService(*audio->Regex(), audioService);
  server->addService(*channels->Regex(), channelsService);
  server->addService(*events->Regex(), eventsService);
  server->addService(*info->Regex(), infoService); 
  server->addService(*osd->Regex(), osdService);  
  server->addService(*recordings->Regex(), recordingsService);
  server->addService(*remote->Regex(), remoteService);
  server->addService(*searchtimers->Regex(), searchTimersService); 
  server->addService(*timers->Regex(), timersService);

  try {
    loop.run();
  } catch ( const std::exception& e) {
    esyslog("restfulapi: starting services failed: /%s/", e.what());
  }
  int now = time(NULL);
  esyslog("restfulapi: server thread end: /%i/", now);

  dsyslog("restfulapi: server thread ended (pid=%d)", getpid());
}
