#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include "tools.h"

#include <vdr/channels.h>

class ChannelsResponder : public cxxtools::http::Responder
{
  public:
    explicit ChannelsResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyChannels(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyImage(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& replay);
    virtual void replyGroups(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<ChannelsResponder> ChannelsService;

struct SerChannel
{
  cxxtools::String Name;
  int Number;
  cxxtools::String ChannelId;
  int Transponder;
  cxxtools::String Image;
  cxxtools::String Stream;
  cxxtools::String Group;
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

class ChannelList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    ChannelList(std::ostream* _out);
    ~ChannelList();
    virtual void init() { };
    virtual void addChannel(cChannel* channel, std::string group, std::string image) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlChannelList : ChannelList
{
  public:
    HtmlChannelList(std::ostream* _out) : ChannelList(_out) { };
    ~HtmlChannelList() { };
    virtual void init();
    virtual void addChannel(cChannel* channel, std::string group, std::string image);
    virtual void finish();
};

class JsonChannelList : ChannelList
{
  private:
    std::vector < struct SerChannel > serChannels;
  public:
    JsonChannelList(std::ostream* _out) : ChannelList(_out) { };
    ~JsonChannelList() { };
    virtual void addChannel(cChannel* channel, std::string group, std::string image);
    virtual void finish();
};

class XmlChannelList : ChannelList
{
  public:
    XmlChannelList(std::ostream* _out) : ChannelList(_out) { };
    ~XmlChannelList() { };
    virtual void init();
    virtual void addChannel(cChannel* channel, std::string group, std::string image);
    virtual void finish();
};

class ChannelGroupList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    ChannelGroupList(std::ostream* _out);
    ~ChannelGroupList();
    virtual void init() { };
    virtual void addGroup(std::string group) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class HtmlChannelGroupList : ChannelGroupList
{
  public:
    HtmlChannelGroupList(std::ostream* _out) : ChannelGroupList(_out) { };
    ~HtmlChannelGroupList() { };
    virtual void init();
    virtual void addGroup(std::string group);
    virtual void finish();
};

class JsonChannelGroupList : ChannelGroupList
{
  private:
    std::vector< cxxtools::String > groups;
  public:
    JsonChannelGroupList(std::ostream* _out) : ChannelGroupList(_out) { };
    ~JsonChannelGroupList() { };
    virtual void init() { };
    virtual void addGroup(std::string group);
    virtual void finish();
};

class XmlChannelGroupList : ChannelGroupList
{
  public:
    XmlChannelGroupList(std::ostream* _out) : ChannelGroupList(_out) { };
    ~XmlChannelGroupList() { };
    virtual void init();
    virtual void addGroup(std::string group);
    virtual void finish();
};
