#ifndef __SERVERTHREAD_H
#define __SERVERTHREAD_H

#include <cxxtools/http/server.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/eventloop.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/regex.h>

#include <unistd.h>

#include "info.h"
#include "channels.h"
#include "events.h"
#include "recordings.h"
#include "remote.h"
#include "timers.h"
#include "changestate.h"
#include "eventsstreamthread.h"
#include "osd.h"
#include "searchtimers.h"
#include "epgsearch.h"
#include "scraper2vdr.h"
#include "wirbelscan.h"
#include "webapp.h"
#include "femon.h"

using namespace std;

class cServerThread : public cThread {
private:
    bool active;
    std::string listenIp;
    unsigned short int listenPort;
    cxxtools::EventLoop loop;
    cxxtools::http::Server *server;
    EventsStreamThread eventsStreamThread;
    void Action(void);
    WebappService webappService;
    RestfulServices* services;

public:
    cServerThread() { };
    ~cServerThread();
    void Initialize();
    void StartUpdate();
    bool isActive() { return active; };
    void Stop();
    void addWebappService(std::string name);
};

#endif
