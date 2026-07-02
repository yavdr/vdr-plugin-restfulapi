#include "serverthread.h"

void cServerThread::Initialize()
{

	SetDescription("Restfulapi Serverthread");
  active = false; 

  listenIp = Settings::get()->Ip();
  listenPort = Settings::get()->Port();

  eventsStreamThread.Initialize(listenIp, static_cast<unsigned short int>(listenPort + 1));
  eventsStreamThread.Start();

  isyslog("create server");
  server = new cxxtools::http::Server(loop, listenIp, listenPort);

  services = RestfulServices::get();

  }

void cServerThread::Stop() {
  eventsStreamThread.Stop();
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

  InfoService infoService;
  ChannelsService channelsService;
  EventsService eventsService;
  RecordingsService recordingsService;
  RemoteService remoteService;
  TimersService timersService;
  ChangeStateService changeStateService;
  OsdService osdService;
  SearchTimersService searchTimersService;
  ScraperService scraperService;
  WirbelscanService wirbelscanService;
  FemonService femonService;
  
  RestfulService* info = new RestfulService("/info", true, 1);
  RestfulService* channels = new RestfulService("/channels", true, 1);
  RestfulService* channelGroups = new RestfulService("/channels/groups", true, 1, channels);
  RestfulService* channelImage = new RestfulService("/channels/image", true, 1, channels);
  RestfulService* events = new RestfulService("/events", true, 1);
  RestfulService* eventsImage = new RestfulService("/events/image", true, 1, events);
  RestfulService* eventsSearch = new RestfulService("/events/search", false, 1, events);
  RestfulService* recordings = new RestfulService("/recordings", true, 1);
  RestfulService* recordingsCut = new RestfulService("/recordings/cut", true, 1, recordings);
  RestfulService* recordingsMarks = new RestfulService("/recordings/marks", true, 1, recordings);
  RestfulService* remote = new RestfulService("/remote", true, 1);
  RestfulService* timers = new RestfulService("/timers", true, 1);
  RestfulService* changeState = new RestfulService("/change-state", true, 1);
  RestfulService* osd = new RestfulService("/osd", true, 1);
  RestfulService* searchtimers = new RestfulService("/searchtimers", false, 1);
  RestfulService* scraper = new RestfulService("/scraper", true, 1);
  RestfulService* wirbelscan = new RestfulService("/wirbelscan", true, 1);
  RestfulService* wirbelscanCountries = new RestfulService("/wirbelscan/countries", true, 1, wirbelscan);
  RestfulService* femon = new RestfulService("/femon", true, 1);
  
  services->appendService(info);
  services->appendService(channels);
  services->appendService(channelGroups);
  services->appendService(channelImage);
  services->appendService(events);
  services->appendService(eventsImage);
  services->appendService(eventsSearch);
  services->appendService(recordings);
  services->appendService(recordingsCut);
  services->appendService(recordingsMarks);
  services->appendService(remote);
  services->appendService(timers);
  services->appendService(changeState);
  services->appendService(osd);
  services->appendService(searchtimers);
  services->appendService(scraper);
  services->appendService(wirbelscan);
  services->appendService(wirbelscanCountries);
  services->appendService(femon);

  server->addService(std::move(*info->Regex()), infoService);
  server->addService(std::move(*channels->Regex()), channelsService);
  server->addService(std::move(*events->Regex()), eventsService);
  server->addService(std::move(*recordings->Regex()), recordingsService);
  server->addService(std::move(*remote->Regex()), remoteService);
  server->addService(std::move(*timers->Regex()), timersService);
  server->addService(std::move(*changeState->Regex()), changeStateService);
  server->addService(std::move(*osd->Regex()), osdService);
  server->addService(std::move(*searchtimers->Regex()), searchTimersService);
  server->addService(std::move(*scraper->Regex()), scraperService);
  server->addService(std::move(*wirbelscan->Regex()), wirbelscanService);
  server->addService(std::move(*femon->Regex()), femonService);

  map<string, string> webapps = Settings::get()->Webapps();
  map<string, string>::iterator it;

  for (it = webapps.begin(); it != webapps.end(); it++) {
      addWebappService(it->first);
  }

  try {
    loop.run();
  } catch ( const std::exception& e) {
    esyslog("restfulapi: starting services failed: /%s/", e.what());
  }
  int now = time(NULL);
  esyslog("restfulapi: server thread end: /%i/", now);

  dsyslog("restfulapi: server thread ended (pid=%d)", getpid());
}

void cServerThread::addWebappService(string name) {

  string path = "/" + name;
  vector< RestfulService* > restfulservices = services->Services(true, true);
  vector< RestfulService* >::iterator it = restfulservices.begin();
  vector< RestfulService* >::iterator end = restfulservices.end();
  bool occupied = false;
  int i=0;

  for (; it != end; it++) {

      if (restfulservices[i]->Path() == path) {
	  occupied = true;
      }
      i++;
  }

  if (false == occupied) {
      RestfulService* service = new RestfulService(path, true, 1);
      services->appendService(service);
      server->addService(std::move(*service->Regex()), webappService);
      esyslog("restfulapi: webapp service '%s' added", name.c_str());
  } else {
      esyslog("restfulapi: could not add service '%s' because it already exists", name.c_str());
  }
};
