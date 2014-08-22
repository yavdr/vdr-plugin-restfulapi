#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/query_params.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/regex.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include "tools.h"

class AudioResponder : public cxxtools::http::Responder
{
  public:
    explicit AudioResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service) { }
    ~AudioResponder() { }
    
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<AudioResponder> AudioService;

struct SerTrackList
{
  int Count;
  std::vector< struct SerTrack > track;
};

struct SerAudio
{
  int Volume;
  int Mute; 
  int Number;
  cxxtools::String Description;
  cxxtools::String Channel;
  SerTrackList Tracks;
};

struct SerTrack
{
  int Number;
  cxxtools::String Description;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerAudio& a);
void operator<<= (cxxtools::SerializationInfo& si, const SerTrack& t);
void operator<<= (cxxtools::SerializationInfo& si, const SerTrackList& tl);

class AudioList : public BaseList
{
  protected:
    StreamExtension *s;
  public:
    explicit AudioList(std::ostream* _out);
    virtual ~AudioList();
    virtual void init() { };
    virtual void addContent() { };
    virtual void finish() { };
};

class HtmlAudioList : AudioList
{
  public:
    explicit HtmlAudioList(std::ostream* _out) : AudioList(_out) { };
    ~HtmlAudioList() { };
    virtual void init();
    virtual void addContent();
    virtual void finish();
};

class JsonAudioList : AudioList
{
  private:
    std::vector < struct SerAudio > serAudios;
  public:
    explicit JsonAudioList(std::ostream* _out) : AudioList(_out) { };
    ~JsonAudioList() { };
    virtual void addContent();
    virtual void finish();
};

class XmlAudioList : AudioList
{
  public:
    explicit XmlAudioList(std::ostream* _out) : AudioList(_out) { };
    ~XmlAudioList() { };
    virtual void init();
    virtual void addContent();
    virtual void finish();
};
