#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "tools.h"
#include "scraper2vdr.h"

#include <vdr/cutter.h>
#include <vdr/recording.h>
#include <vdr/videodir.h>

class SyncMap
{
private:
  std::string id;
  std::map<std::string, std::string> serverMap;
  std::map<std::string, std::string> clientMap;
  FILE* getSyncFile(bool write);
  void clear(bool server);
public:
  SyncMap(QueryHandler q, bool overrideFormat = false);
  ~SyncMap() {};
  void load();
  void setClientMap(std::map<std::string, std::string> clientMap);
  void write(bool server = true);
  std::map<std::string, std::string> getUpdates();
  void add(std::string filename, std::string hash);
  void erase(std::string filename);
  void log(bool server);
  bool active();
};

class RecordingList : public BaseList
{
  protected:
    bool read_marks;
    int total;
    StreamExtension *s;
    Scraper2VdrService sc;
  public:
    RecordingList(std::ostream* _out, bool _read_marks);
    virtual ~RecordingList();
    virtual void init() { };
    virtual void addRecording(const cRecording* recording, int nr, SyncMap*, std::string sync_action, bool add_hash = false) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class RecordingsResponder : public cxxtools::http::Responder
{
private:
  const char* CT_JSON;
  const char* CT_HTML;
  const char* CT_XML;
  public:
    explicit RecordingsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service) {
	CT_JSON = "application/json; charset=utf-8";
	CT_HTML = "text/html; charset=utf-8";
	CT_XML = "text/xml; charset=utf-8";
      }

    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void deleteRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void showRecordings(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void saveMarks(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void deleteMarks(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void cutRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void showCutterStatus(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void playRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void rewindRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void moveRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyEditedFileName(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replyUpdates(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void replySyncList(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    void sendSyncList(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, SyncMap* sync_map);
    void initServerList(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply, SyncMap* sync_map);
    const cRecording* getRecordingByRequest(QueryHandler q);
    cRecording* getRecordingByRequestWrite(QueryHandler q);
    RecordingList* getRecordingList(std::ostream& out, QueryHandler q, cxxtools::http::Reply& reply, bool _read_marks);
};

typedef cxxtools::http::CachedService<RecordingsResponder> RecordingsService;

class SerMarks
{
  public:
    std::vector< std::string > marks;
};

struct SerRecording
{
  int Number;
  cxxtools::String Name;
  cxxtools::String FileName;
  cxxtools::String RelativeFileName;
  cxxtools::String Inode;
  cxxtools::String ChannelID;
  bool IsNew;
  bool IsEdited;
  bool IsPesRecording;
  int Duration;
  int FileSizeMB;
  double FramesPerSecond;
  SerMarks Marks;
  cxxtools::String EventTitle;
  cxxtools::String EventShortText;
  cxxtools::String EventDescription;
  int EventStartTime;
  int EventDuration;
  cxxtools::String Aux;
  struct SerAdditionalMedia AdditionalMedia;
  cxxtools::String SyncAction;
  cxxtools::String hash;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p);

class HtmlRecordingList : RecordingList
{
  public:
    HtmlRecordingList(std::ostream* _out, bool _read_marks) : RecordingList(_out, _read_marks) { };
    ~HtmlRecordingList() { };
    virtual void init();
    virtual void addRecording(const cRecording* recording, int nr, SyncMap* sync_map, std::string sync_action, bool add_hash = false);
    virtual void finish();
};

class JsonRecordingList : RecordingList
{
  private:
    std::vector < struct SerRecording > serRecordings;
  public:
    JsonRecordingList(std::ostream* _out, bool _read_marks) : RecordingList(_out, _read_marks) { };
    ~JsonRecordingList() { };
    virtual void addRecording(const cRecording* recording, int nr, SyncMap* sync_map, std::string sync_action, bool add_hash = false);
    virtual void finish();
};

class XmlRecordingList : RecordingList
{
  public:
    XmlRecordingList(std::ostream* _out, bool _read_marks) : RecordingList(_out, _read_marks) { };
    ~XmlRecordingList() { };
    virtual void init();
    virtual void addRecording(const cRecording* recording, int nr, SyncMap* sync_map, std::string sync_action, bool add_hash = false);
    virtual void finish();
};








