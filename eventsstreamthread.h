#ifndef __EVENTSSTREAMTHREAD_H
#define __EVENTSSTREAMTHREAD_H

#include <stdint.h>

#include <atomic>
#include <string>
#include <vector>

#include <vdr/thread.h>

class EventsStreamThread : public cThread
{
private:
    std::atomic<bool> active;
    std::atomic<int> serverFd;
    std::string listenIp;
    unsigned short listenPort;

    void Action(void) override;

    bool createServerSocket();
    void handleClient(int clientFd);
    bool readHttpRequest(int clientFd, std::string& requestData);

    void sendOptionsResponse(int clientFd);
    void sendNotFoundResponse(int clientFd);
    void sendMethodNotAllowedResponse(int clientFd);
    bool sendAll(int clientFd, const std::string& data);

    bool sendStreamHeaders(int clientFd);
    bool sendHello(int clientFd);
    bool sendHeartbeat(int clientFd);
    bool sendChange(int clientFd, uint64_t eventId, const std::vector<std::string>& domains);

public:
    EventsStreamThread();
    ~EventsStreamThread();

    void Initialize(const std::string& ip, unsigned short port);
    void Stop();
};

#endif
