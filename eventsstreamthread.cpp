#include "eventsstreamthread.h"

#include "changestatetracker.h"

#include <vdr/tools.h>

#include <algorithm>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace
{
    std::string jsonDomainArray(const std::vector<std::string>& domains)
    {
        std::ostringstream out;
        out << "[";

        for (std::size_t i = 0; i < domains.size(); ++i)
        {
            if (i > 0)
            {
                out << ",";
            }

            out << "\"" << domains[i] << "\"";
        }

        out << "]";
        return out.str();
    }

    uint64_t maxChangeId(
        uint64_t channels,
        uint64_t recordings,
        uint64_t timers,
        uint64_t events)
    {
        return std::max(std::max(channels, recordings), std::max(timers, events));
    }

    bool pathMatchesEventStream(const std::string& path)
    {
        return path == "/eventstream" || path == "/eventstream/";
    }
}

EventsStreamThread::EventsStreamThread()
    : active(false),
      serverFd(-1),
      listenPort(0)
{
}

EventsStreamThread::~EventsStreamThread()
{
    Stop();
}

void EventsStreamThread::Initialize(const std::string& ip, unsigned short port)
{
    SetDescription("Restfulapi EventStreamThread");

    listenIp = ip;
    listenPort = port;
}

void EventsStreamThread::Stop()
{
    active.store(false);

    const int fd = serverFd.exchange(-1);
    if (fd >= 0)
    {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    Cancel(1);
}

void EventsStreamThread::Action(void)
{
    active.store(true);

    if (!createServerSocket())
    {
        active.store(false);
        return;
    }

    isyslog(
        "restfulapi: eventstream listening on %s:%u",
        listenIp.c_str(),
        static_cast<unsigned int>(listenPort));

    while (active.load())
    {
        const int fd = serverFd.load();
        if (fd < 0)
        {
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);

        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        const int result = select(fd + 1, &readSet, NULL, NULL, &timeout);

        if (!active.load())
        {
            break;
        }

        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            esyslog("restfulapi: eventstream select failed: %s", strerror(errno));
            break;
        }

        if (result == 0)
        {
            continue;
        }

        sockaddr_in clientAddress;
        socklen_t clientLength = sizeof(clientAddress);
        const int clientFd = accept(fd, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);

        if (clientFd < 0)
        {
            if (active.load())
            {
                esyslog("restfulapi: eventstream accept failed: %s", strerror(errno));
            }
            continue;
        }

        handleClient(clientFd);
        close(clientFd);
    }

    const int fd = serverFd.exchange(-1);
    if (fd >= 0)
    {
        close(fd);
    }

    isyslog("restfulapi: eventstream stopped");
}

bool EventsStreamThread::createServerSocket()
{
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        esyslog("restfulapi: eventstream socket failed: %s", strerror(errno));
        return false;
    }

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(listenPort);

    if (listenIp.empty() || listenIp == "0.0.0.0")
    {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (inet_pton(AF_INET, listenIp.c_str(), &address.sin_addr) != 1)
    {
        esyslog("restfulapi: eventstream invalid listen ip: %s", listenIp.c_str());
        close(fd);
        return false;
    }

    if (bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    {
        esyslog(
            "restfulapi: eventstream bind failed on %s:%u: %s",
            listenIp.c_str(),
            static_cast<unsigned int>(listenPort),
            strerror(errno));
        close(fd);
        return false;
    }

    if (listen(fd, 4) < 0)
    {
        esyslog("restfulapi: eventstream listen failed: %s", strerror(errno));
        close(fd);
        return false;
    }

    serverFd.store(fd);
    return true;
}

void EventsStreamThread::handleClient(int clientFd)
{
    std::string requestData;
    if (!readHttpRequest(clientFd, requestData))
    {
        return;
    }

    std::istringstream requestStream(requestData);
    std::string method;
    std::string path;
    std::string protocol;

    requestStream >> method >> path >> protocol;

    if (method == "OPTIONS")
    {
        sendOptionsResponse(clientFd);
        return;
    }

    if (method != "GET")
    {
        sendMethodNotAllowedResponse(clientFd);
        return;
    }

    if (!pathMatchesEventStream(path))
    {
        sendNotFoundResponse(clientFd);
        return;
    }

    isyslog("restfulapi: eventstream client connected");

    if (!sendStreamHeaders(clientFd))
    {
        return;
    }

    uint64_t lastChannels = StateChangeTracker::LastChannelsUpdate();
    uint64_t lastRecordings = StateChangeTracker::LastRecordingsUpdate();
    uint64_t lastTimers = StateChangeTracker::LastTimersUpdate();
    uint64_t lastEvents = StateChangeTracker::LastEventsUpdate();

    if (!sendHello(clientFd))
    {
        return;
    }

    int heartbeatTicks = 0;

    while (active.load())
    {
        for (int i = 0; i < 10 && active.load(); ++i)
        {
            usleep(100000);
        }

        if (!active.load())
        {
            break;
        }

        const uint64_t currentChannels = StateChangeTracker::LastChannelsUpdate();
        const uint64_t currentRecordings = StateChangeTracker::LastRecordingsUpdate();
        const uint64_t currentTimers = StateChangeTracker::LastTimersUpdate();
        const uint64_t currentEvents = StateChangeTracker::LastEventsUpdate();

        std::vector<std::string> domains;

        if (currentChannels != lastChannels)
        {
            domains.push_back("channels");
            lastChannels = currentChannels;
        }

        if (currentRecordings != lastRecordings)
        {
            domains.push_back("recordings");
            lastRecordings = currentRecordings;
        }

        if (currentTimers != lastTimers)
        {
            domains.push_back("timers");
            lastTimers = currentTimers;
        }

        if (currentEvents != lastEvents)
        {
            domains.push_back("events");
            lastEvents = currentEvents;
        }

        if (!domains.empty())
        {
            heartbeatTicks = 0;

            if (!sendChange(
                    clientFd,
                    maxChangeId(currentChannels, currentRecordings, currentTimers, currentEvents),
                    domains))
            {
                break;
            }

            continue;
        }

        ++heartbeatTicks;
        if (heartbeatTicks >= 5)
        {
            heartbeatTicks = 0;

            if (!sendHeartbeat(clientFd))
            {
                break;
            }
        }
    }

    isyslog("restfulapi: eventstream client disconnected");
}

bool EventsStreamThread::readHttpRequest(int clientFd, std::string& requestData)
{
    timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char buffer[1024];

    while (requestData.size() < 8192)
    {
        const ssize_t received = recv(clientFd, buffer, sizeof(buffer), 0);

        if (received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return !requestData.empty();
        }

        if (received == 0)
        {
            return !requestData.empty();
        }

        requestData.append(buffer, static_cast<std::size_t>(received));

        if (requestData.find("\r\n\r\n") != std::string::npos ||
            requestData.find("\n\n") != std::string::npos)
        {
            return true;
        }
    }

    return true;
}

void EventsStreamThread::sendOptionsResponse(int clientFd)
{
    sendAll(
        clientFd,
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: accept, authorization\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n");
}

void EventsStreamThread::sendNotFoundResponse(int clientFd)
{
    sendAll(
        clientFd,
        "HTTP/1.1 404 Not Found\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n");
}

void EventsStreamThread::sendMethodNotAllowedResponse(int clientFd)
{
    sendAll(
        clientFd,
        "HTTP/1.1 405 Method Not Allowed\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Allow: GET, OPTIONS\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n");
}

bool EventsStreamThread::sendAll(int clientFd, const std::string& data)
{
    const char* cursor = data.c_str();
    std::size_t remaining = data.size();

    while (remaining > 0)
    {
        const ssize_t sent = send(clientFd, cursor, remaining, MSG_NOSIGNAL);

        if (sent <= 0)
        {
            return false;
        }

        cursor += sent;
        remaining -= static_cast<std::size_t>(sent);
    }

    return true;
}

bool EventsStreamThread::sendStreamHeaders(int clientFd)
{
    return sendAll(
        clientFd,
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
        "Access-Control-Allow-Headers: accept, authorization\r\n"
        "Content-Type: text/event-stream; charset=utf-8\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "X-Accel-Buffering: no\r\n"
        "\r\n");
}

bool EventsStreamThread::sendHello(int clientFd)
{
    std::ostringstream out;

    out
        << "event: hello\n"
        << "data: {\"source\":\"restfulapi\",\"bootID\":\""
        << StateChangeTracker::bootID
        << "\",\"domains\":[]}\n"
        << "\n";

    return sendAll(clientFd, out.str());
}

bool EventsStreamThread::sendHeartbeat(int clientFd)
{
    return sendAll(clientFd, ": heartbeat\n\n");
}

bool EventsStreamThread::sendChange(
    int clientFd,
    uint64_t eventId,
    const std::vector<std::string>& domains)
{
    std::ostringstream out;

    out
        << "event: vdr-change\n"
        << "id: " << eventId << "\n"
        << "data: {\"source\":\"restfulapi\",\"domains\":"
        << jsonDomainArray(domains)
        << "}\n"
        << "\n";

    return sendAll(clientFd, out.str());
}
