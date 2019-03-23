#include <glob.h>
#include <list>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <exception>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <cxxtools/regex.h>
#include <cxxtools/string.h>
#include <cxxtools/utf8codec.h>
#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/query_params.h>
#include <cxxtools/serializationinfo.h> // only AdditionalMedia
#include <cxxtools/md5.h>
#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/recording.h>
#include <vdr/plugin.h>
#include "utf8_checked.h"
#include "jsonparser.h"

#define LOWINT 2147483647;

//some defines for inotify memory allocation
#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#ifndef RESTFULAPI_EXTENSIONS
#define RESTFULAPI_EXTENSIONS

#ifndef DOCUMENT_ROOT
#define DOCUMENT_ROOT "/var/lib/vdr/plugins/restfulapi/"
#endif

class Settings
{
  private:
    int port;
    std::string ip;
    std::string epgimage_dir;
    std::string channellogo_dir;
    std::string webapp_dir;
    std::map<std::string, std::string> webapps;
    std::string cache_dir;
    std::string conf_dir;
    std::string webapp_filetypes_filename;
    std::map<std::string, std::string> webapp_file_types;
    bool activateHeaders;
    void addWebapp(std::string path);
  public:
    Settings() { initDefault(); }
    ~Settings() { };
    static Settings* get();
    void initDefault();
    int Port() { return port; }
    std::string Ip() { return ip; }
    std::string EpgImageDirectory() { return epgimage_dir; }
    std::string ChannelLogoDirectory() { return channellogo_dir; }
    std::string WebappDirectory();
    std::map<std::string, std::string> Webapps() { return webapps; }
    std::string CacheDirectory() { return cache_dir; }
    std::string ConfDirectory() { return conf_dir; }
    std::string WebAppFileTypesFilename() { return webapp_filetypes_filename; }
    bool InitWebappFileTypes();
    bool AddWebappFileType(std::string ext, std::string type);
    std::map<std::string, std::string> WebappFileTypes();
    bool Headers() { return activateHeaders; }
    bool SetPort(std::string v);
    bool SetIp(std::string v);
    bool SetEpgImageDirectory(std::string v);
    bool SetChannelLogoDirectory(std::string v);
    bool SetWebappDirectory(std::string v);
    bool SetCacheDir(std::string v);
    bool SetConfDir(std::string v);
    bool SetHeaders(std::string v);
};

class StreamExtension
{
  private:
    std::ostream *_out;
  public:
    StreamExtension(std::ostream *out);
    ~StreamExtension() { };
    std::ostream* getBasicStream();
    void write(const char* str);
    void write(cString& str) { write((const char*)str); }
    void writeHtmlHeader(std::string title);
    void writeXmlHeader();
    bool writeBinary(std::string path);
};

class HtmlHeader
{
  private:
    std::string _title;
    std::string _onload;
    std::vector< std::string > _stylesheets;
    std::vector< std::string > _scripts;
    std::vector< std::string > _metatags;
  public:
    HtmlHeader() { }
    ~HtmlHeader() { }
    void Title(std::string title) { _title = title; }
    std::string Title() { return _title; }

    void OnLoad(std::string onload) { _onload = onload; }
    std::string OnLoad() { return _onload; }

    void Stylesheet(std::string stylesheet) { _stylesheets.push_back(stylesheet); }
    std::vector< std::string >& Stylesheet() { return _stylesheets; }

    void Script(std::string script) { _scripts.push_back(script); }
    std::vector< std::string >& Scripts() { return _scripts; }

    void MetaTag(std::string metatag) { _metatags.push_back(metatag); }
    std::vector< std::string >& MataTags() { return _metatags; }

    void ToStream(StreamExtension* se);
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
    static const int WEBAPPFILETYPES = 0x03;
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
    FileNotifier notifierWebappFileTypes;
  public:
    FileCaches() {
         cacheEventImages();
         cacheChannelLogos();
         cacheWebappFileTypes();
         notifierEvents.Initialize(FileNotifier::EVENTS);
         notifierChannels.Initialize(FileNotifier::CHANNELS);
         notifierWebappFileTypes.Initialize(FileNotifier::WEBAPPFILETYPES);
      };
    ~FileCaches() { };
    static FileCaches* get();
    void cacheEventImages();
    void cacheChannelLogos();
    void cacheWebappFileTypes();
    void searchEventImages(int eventid, std::vector< std::string >& files);
    std::string searchChannelLogo(const cChannel *channel);
    void addEventImage(std::string file);
    void addChannelLogo(std::string file);
    void removeEventImage(std::string file);
    void removeChannelLogo(std::string file);
    void stopNotifier() {
      notifierEvents.Stop();
      notifierChannels.Stop();
      notifierWebappFileTypes.Stop();
    };
};

class FileExtension {
  public:
    static FileExtension* get();
    struct tm* getModifiedTm(std::string path);
    time_t getModifiedTime(std::string path);
    void addModifiedHeader(std::string path, cxxtools::http::Reply& reply);
    time_t getModifiedSinceTime(cxxtools::http::Request& request);
    const char* getLocale();
    bool exists(std::string path);
};

class VdrExtension
{
  private:
  public:
    static const cChannel* getChannel(int number);
    static const cChannel* getChannel(std::string id);
    static const cTimer* getTimer(std::string id);
    static cTimer* getTimerWrite(std::string id);
    static std::string getTimerID(const cTimer* timer);
    static int scanForFiles(const std::string wildcardpath, std::vector< std::string >& files);
    static bool doesFileExistInFolder(std::string wildcardpath, std::string filename);
    static bool IsRadio(const cChannel* channel);
    static bool IsRecording(const cRecording* recording);
    static const cTimer* TimerExists(const cEvent* event);
    static std::vector< const cTimer* > SortedTimers();
    static bool CompareTimers(const cTimer* timer1, const cTimer* timer2);
    static int RecordingLengthInSeconds(const cRecording* recording);
    static const cEvent* GetEventById(tEventID eventID, const cChannel* channel);
    static std::string getRelativeVideoPath(const cRecording* recording);
    static cEvent* getCurrentEventOnChannel(const cChannel* channel);
    static std::string getVideoDiskSpace();
    static std::string FileSystemExchangeChars(std::string const & s, bool ToFileSystem);
    static std::string MoveRecording(cRecording const * recording, std::string const & name, bool copy = false);
    static bool MoveDirectory(std::string const & sourceDir, std::string const & targetDir, bool copy = false);
    static cDvbDevice* getDevice(int index);
};

class VdrMarks
{
  private:
    std::string cutComment(std::string str);
    bool validateMark(std::string mark);
    std::string getPath(const cRecording* recording);
    bool parseLine(std::vector< std::string >& marks, std::string line);
  public:
    static VdrMarks* get();
    std::vector< std::string > readMarks(const cRecording* recording);
    bool saveMarks(const cRecording* recording, std::vector< std::string > marks);
    bool deleteMarks(const cRecording* recording);
};

class StringExtension
{
  public:
    static std::string itostr(int i);
    static int strtoi(std::string str);
    static std::string replace(std::string const& text, std::string const& substring, std::string const& replacement);
    static std::string encodeToXml(const std::string &str);
    static std::string encodeToXml(cxxtools::String &str);
    static cxxtools::String encodeToJson(const std::string &str);
    static cxxtools::String encodeToJson(cxxtools::String &str);
    static cxxtools::String UTF8Decode(std::string str);
    static std::string toLowerCase(std::string str);
    static std::string trim(std::string str);
    static std::vector< std::string > split(std::string str, std::string s);
    static std::string join(std::vector<std::string> in, std::string glue);
    static std::string join(std::map<std::string,std::string> in, std::string glue, bool keys = false);
    static std::string timeToString(time_t time);
    static std::string dateToString(time_t time);
    static std::string addZeros(int value, int digits);
    static std::string toString(cxxtools::String value);
    static std::string toString(cString value);
};

class QueryHandler
{
  private:
    std::string _url;
    std::string _service;
    std::vector< std::string > _params;
    cxxtools::QueryParams _options;
    cxxtools::QueryParams _body;
    JsonParser jsonParser;
    JsonObject* jsonObject;
    std::string fixUrl(std::string url);
    void parseRestParams(std::string params);
    std::string getJsonString(std::string name);
    int         getJsonInt(std::string name);
    bool        getJsonBool(std::string name);
    std::string _format;
  public:
    QueryHandler(std::string service, cxxtools::http::Request& request);
    ~QueryHandler();
    bool has(std::string name);
    bool hasJson(std::string name);
    bool hasOption(std::string name);
    bool hasBody(std::string name);
    std::string getParamAsString(int level);              //Parameters are part of the url (the rest after you cut away the service path)
    std::string getParamAsRecordingPath();
    std::string getOptionAsString(std::string name);      //Options are the normal url query parameters after the question mark
    std::string getBodyAsString(std::string name);        //Are variables in the body of the http-request -> for now only html/json are supported, xml is not implemented (!)
    int getParamAsInt(int level);
    int getOptionAsInt(std::string name);
    bool getOptionAsBool(std::string name);
    int getBodyAsInt(std::string name);
    bool getBodyAsBool(std::string name);
    JsonArray* getBodyAsArray(std::string name);
    bool isFormat(std::string format);
    std::string getFormat() { return _format; }
    static void addHeader(cxxtools::http::Reply& reply);
    std::vector< std::string > getBodyAsStringArray(std::string name);
    std::vector< int > getBodyAsIntArray(std::string name);
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
    virtual ~BaseList() { };
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
    virtual ~BasicOsd() { };
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
    virtual ~BaseTask() { };
    int Created() { return created; };
};

class TaskScheduler
{
  protected:
    std::list<BaseTask*> tasks;
    tChannelID _channel;
    const cRecording* _recording;
    cMutex     _channelMutex;
    bool _bRewind;
  public:
    TaskScheduler() { _channel = tChannelID::InvalidID; _recording = NULL; };
    ~TaskScheduler();
    static TaskScheduler* get();
    void AddTask(BaseTask* task) { tasks.push_back(task); };
    void DoTasks();
    void SwitchableChannel(tChannelID channel);
    tChannelID SwitchableChannel();
    void SwitchableRecording(const cRecording* recording) { _recording = recording; }
    const cRecording* SwitchableRecording() { return _recording; }
    void SetRewind(bool bRewind) { _bRewind = bRewind; }
    bool IsRewind() { return _bRewind; }
};

#endif

