/*
 * serverthread.cpp: JSONAPI plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "serverthread.h"

cServerThread::cServerThread ()
{
  active = false; 

  listenIp = "0.0.0.0";
  listenPort = 8002;

  isyslog("create server");
  server = new cxxtools::http::Server(loop, listenIp, listenPort);
}

void cServerThread::Stop() {
  active = false;
  loop.exit();
  delete server;
}

cServerThread::~cServerThread ()
{
  Cancel(0);
}

void cServerThread::Action(void)
{
  active = true;

  ChannelsService channelsService;
  EventsService eventsService;
  RecordingsService recordingsService;
  RemoteService remoteService;
  TimersService timersService;

  cxxtools::Regex channelsRegex("/channels*");
  cxxtools::Regex eventsRegex("/events*");
  cxxtools::Regex recordingsRegex("/recordings*");
  cxxtools::Regex remoteRegex("/remote*");
  cxxtools::Regex timersRegex("/timers*");

  server->addService(channelsRegex, channelsService);
  server->addService(eventsRegex, eventsService);
  server->addService(recordingsRegex, recordingsService);
  server->addService(remoteRegex, remoteService);
  server->addService(timersRegex, timersService);

  try {
    loop.run();
  } catch ( const std::exception& e) {
    esyslog("restfulapi: starting services failed: /%s/", e.what());
  }

  dsyslog("restfulapi: server thread ended (pid=%d)", getpid());
}
