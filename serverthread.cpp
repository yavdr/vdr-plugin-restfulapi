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
  server->isRestful(true);

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

  server->addService(/*(const cxxtools::Regex*)new cxxtools::Regex(*/"/channels"/**")*/, channelsService);
  server->addService(/*(const cxxtools::Regex*)new cxxtools::Regex(*/"/events"/**")*/, eventsService);
  server->addService(/*(const cxxtools::Regex*)new cxxtools::Regex(*/"/recordings"/**")*/, recordingsService);
  server->addService(/*(const cxxtools::Regex*)new cxxtools::Regex(*/"/timers"/**")*/, timersService);

  loop.run();

  dsyslog("restfulapi: server thread ended (pid=%d)", getpid());
}
