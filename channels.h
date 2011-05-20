#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include "tools.h"

#include "vdr/channels.h"

class ChannelsResponder : public cxxtools::http::Responder
{
  public:
    explicit ChannelsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<ChannelsResponder> ChannelsService;

struct SerChannel
{
  cxxtools::String Name;
  int Number;
  int Transponder;
  cxxtools::String Stream;
  bool IsAtsc;
  bool IsCable;
  bool IsTerr;
  bool IsSat;
};

struct SerChannels
{
  std::vector < struct SerChannel > channel;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerChannel& c);
void operator<<= (cxxtools::SerializationInfo& si, const SerChannels& c);

class ChannelList
{
  protected:
    std::ostream* out;
  public:
    ChannelList(std::ostream* _out) { out = _out; };
    ~ChannelList() { };
    virtual void addChannel(cChannel* channel) { };
    virtual void finish() { };
};

class HtmlChannelList : ChannelList
{
  public:
    HtmlChannelList(std::ostream* _out);
    ~HtmlChannelList() { };
    virtual void addChannel(cChannel* channel);
    virtual void finish();
};

class JsonChannelList : ChannelList
{
  private:
    std::vector < struct SerChannel > serChannels;
  public:
    JsonChannelList(std::ostream* _out);
    ~JsonChannelList() { };
    virtual void addChannel(cChannel* channel);
    virtual void finish();
};

