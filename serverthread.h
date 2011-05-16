/*
 * serverthread.h: JSONAPI plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __SERVERTHREAD_H
#define __SERVERTHREAD_H

#include <cxxtools/http/server.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/eventloop.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/utf8codec.h>

#include <unistd.h>
#include <vdr/tools.h>
#include <vdr/thread.h>
#include <vdr/recording.h>

#include "data.h"
#include "tools.h"

using namespace std;


class cServerThread : public cThread {
private:
    bool active;
    std::string listenIp;
    unsigned short int listenPort;
    cxxtools::EventLoop loop;
    cxxtools::http::Server *server;
    void Action(void);

public:
    cServerThread();
    ~cServerThread();
    void StartUpdate();
    bool isActive() { return active; };
    void Stop() { active = false; };
};

#endif //__SERVERTHREAD_H
