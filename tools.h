#include <glob.h>
#include <list>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <cxxtools/regex.h>
#include <cxxtools/string.h>
#include <cxxtools/utf8codec.h>
#include <cxxtools/http/request.h>
#include <cxxtools/query_params.h>
#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/plugin.h>
#include "utf8_checked.h"

#define LOWINT 2147483648;

//some defines for inotify memory allocation
#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#ifndef RESTFULAPI_EXTENSIONS
#define RESTFULAPI_EXTENSIONS

class Settings
{
  private:
    int port;
    std::string ip;
    std::string epgimage_dir;
    std::string channellogo_dir;
    std::string cutComment(std::string str);
    bool parseLine(std::string str);
  public:
    Settings() { initDefault(); }
    ~Settings() { };
    static Settings* get();
    void init();
    void initDefault();
    int Port() { return port; }
    std::string Ip() { return ip; }
    std::string EpgImageDirectory() { return epgimage_dir; }
    std::string ChannelLogoDirectory() { return channellogo_dir; }
    bool SetPort(std::string v);
    bool SetIp(std::string v);
    bool SetEpgImageDirectory(std::string v);
    bool SetChannelLogoDirectory(std::string v);
};

class StreamExtension
{
  private:
    std::ostream *_out;
  public:
    StreamExtension(std::ostream *out);
    ~StreamExtension() { };
    std::ostream* getBasicStream();
    void write(std::string str);
    void writeHtmlHeader(std::string css = "");
    void writeXmlHeader();
    bool writeBinary(std::string path);
};

class FileNotifier : public cThread
{
  private:
    int _filedescriptor;
    int _wd;
    int _mode;
    bool active;
    void Action(void);
  public:
    static const int EVENTS = 0x01;
    static const int CHANNELS = 0x02;
    FileNotifier() { active = false; };
    ~FileNotifier();
    void Initialize(int mode);
    void Stop();
    bool isActive() { return active; }
};

class FileCaches
{
  private:
    std::vector< std::string > eventImages;
    std::vector< std::string > channelLogos;
    FileNotifier notifierEvents;
    FileNotifier notifierChannels;
  public:
    FileCaches() {
         cacheEventImages();
         cacheChannelLogos();
         notifierEvents.Initialize(FileNotifier::EVENTS);
         notifierChannels.Initialize(FileNotifier::CHANNELS);
      };
    ~FileCaches() { };
    static FileCaches* get();
    void cacheEventImages();
    void cacheChannelLogos();
    void searchEventImages(int eventid, std::vector< std::string >& files);
    std::string searchChannelLogo(cChannel *channel);
    void addEventImage(std::string file);
    void addChannelLogo(std::string file);
    void removeEventImage(std::string file);
    void removeChannelLogo(std::string file);
    void stopNotifier() {
      notifierEvents.Stop();
      notifierChannels.Stop();
    };
};

class VdrExtension
{
  public:
    static cChannel* getChannel(int number);
    static cChannel* getChannel(std::string id);
    static cTimer* getTimer(std::string id);
    static std::string getTimerID(cTimer* timer);
    static int scanForFiles(const std::string wildcardpath, std::vector< std::string >& files);
    static bool doesFileExistInFolder(std::string wildcardpath, std::string filename);
    static bool IsRadio(cChannel* channel);
};

class StringExtension
{
  public:
    static std::string itostr(int i);
    static int strtoi(std::string str);
    static std::string replace(std::string const& text, std::string const& substring, std::string const& replacement);
    static std::string encodeToXml(const std::string &str);
    static cxxtools::String UTF8Decode(std::string str);
    static std::string toLowerCase(std::string str);
    static std::string trim(std::string str);
    static std::vector< std::string > split(std::string str, std::string s);
};

class QueryHandler
{
  private:
    std::string _url;
    std::string _service;
    std::vector< std::string > _params;
    cxxtools::QueryParams _options;
    cxxtools::QueryParams _body;
    void parseRestParams(std::string params);
    std::string _format;
  public:
    QueryHandler(std::string service, cxxtools::http::Request& request);
    ~QueryHandler();
    std::string getParamAsString(int level);              //Parameters are part of the url (the rest after you cut away the service path)
    std::string getOptionAsString(std::string name);      //Options are the normal url query parameters after the question mark
    std::string getBodyAsString(std::string name);        //Are variables in the body of the http-request -> for now only html is supported!!!
    int getParamAsInt(int level);
    int getOptionAsInt(std::string name);
    int getBodyAsInt(std::string name);
    bool isFormat(std::string format);
    std::string getFormat() { return _format; }
};

class BaseList
{
  protected:
    int iterator;
    int counter;
    int start;
    int limit;
  public:
    BaseList();
    ~BaseList() { };
    virtual void activateLimit(int _start, int _limit);
    virtual bool filtered();
    virtual int Count() { return counter; }
};

class RestfulService
{
  protected:
    cxxtools::Regex* _regex;
    RestfulService* _parent;
    std::string _path;
    bool _internal;
    int _version;
  public:
    RestfulService(std::string path, bool internal = false, int version = 1, RestfulService* parent = NULL);
    ~RestfulService();
    RestfulService* Parent() { return _parent; }
    cxxtools::Regex* Regex() { return _regex; }
    std::string Path() { return _path; }
    bool Internal() { return _internal; }
    int Version() { return _version; }
};

class RestfulServices
{
  protected:
    std::vector< RestfulService* > services;
  public:
    RestfulServices() { };
    ~RestfulServices();
    static RestfulServices* get();
    void appendService(std::string path, bool internal = false, int version = 1, RestfulService* parent = NULL);
    void appendService(RestfulService* service);
    std::vector< RestfulService* > Services(bool internal = false, bool children = false);
};

#endif

#ifndef __RESTFUL_BASICOSD_H
#define __RESTFUL_BASICOSD_H

class BasicOsd
{
  public:
    BasicOsd() { };
    ~BasicOsd() { };
    virtual int Type() { return 0x00; };
};

#endif

#ifndef __RESTFUL_TASKS_H
#define __RESTFUL_TASKS_H

class BaseTask
{
  protected:
    int created; //time in s
  public:
    BaseTask() { created = (int)time(NULL); };
    ~BaseTask() { };
    int Created() { return created; };
};

class TaskScheduler
{
  protected:
    std::list<BaseTask*> tasks;
  public:
    TaskScheduler() { };
    ~TaskScheduler();
    static TaskScheduler* get();
    void AddTask(BaseTask* task) { tasks.push_back(task); };
    void DoTasks();
};

#endif

