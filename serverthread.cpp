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

  loop.run();

  dsyslog("restfulapi: server thread ended (pid=%d)", getpid());
}
