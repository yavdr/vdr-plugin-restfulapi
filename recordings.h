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

#include <vdr/recording.h>

class RecordingsResponder : public cxxtools::http::Responder
{
  public:
    explicit RecordingsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }

    virtual void reply(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void deleteRecording(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void showRecordings(std::ostream&, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<RecordingsResponder> RecordingsService;

int getRecordingDuration(cRecording* m_recording);

struct SerRecording
{
  cxxtools::String Name;
  cxxtools::String FileName;
  bool IsNew;
  bool IsEdited;
  bool IsPesRecording;
  int Duration;
  cxxtools::String EventTitle;
  cxxtools::String EventShortText;
  cxxtools::String EventDescription;
  int EventStartTime;
  int EventDuration;
};

struct SerRecordings
{
  std::vector < struct SerRecording > recording;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerRecording& p);
void operator<<= (cxxtools::SerializationInfo& si, const SerRecordings& p);

class RecordingList
{
  protected:
    StreamExtension *s;
  public:
    RecordingList(std::ostream* _out);
    ~RecordingList();
    virtual void init() { };
    virtual void addRecording(cRecording* recording) { };
    virtual void finish() { };
};

class HtmlRecordingList : RecordingList
{
  public:
    HtmlRecordingList(std::ostream* _out) : RecordingList(_out) { };
    ~HtmlRecordingList() { };
    virtual void init();
    virtual void addRecording(cRecording* recording);
    virtual void finish();
};

class JsonRecordingList : RecordingList
{
  private:
    std::vector < struct SerRecording > serRecordings;
  public:
    JsonRecordingList(std::ostream* _out) : RecordingList(_out) { };
    ~JsonRecordingList() { };
    virtual void addRecording(cRecording* recording);
    virtual void finish();
};

class XmlRecordingList : RecordingList
{
  public:
    XmlRecordingList(std::ostream* _out) : RecordingList(_out) { };
    ~XmlRecordingList() { };
    virtual void init();
    virtual void addRecording(cRecording* recording);
    virtual void finish();
};
