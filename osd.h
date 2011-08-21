#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/arg.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <cxxtools/utf8codec.h>
#include <cxxtools/string.h>
#include "statusmonitor.h"
#include <list>
#include "tools.h"

class OsdResponder : public cxxtools::http::Responder
{
  public:
    explicit OsdResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      { }

    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void printEmptyHtml(std::ostream& out);
    virtual void printTextOsd(std::ostream& out, TextOsd* osd, std::string format, int start_filter, int limit_filter);
};

typedef cxxtools::http::CachedService<OsdResponder> OsdService;

struct SerTextOsdItem
{
  cxxtools::String Content;
  bool Selected;
};

class SerTextOsdItemContainer
{
  public:
    std::vector< struct SerTextOsdItem > items;
};

struct SerTextOsd
{
  cxxtools::String Title;
  cxxtools::String Message;
  cxxtools::String Red;
  cxxtools::String Green;
  cxxtools::String Yellow;
  cxxtools::String Blue;
  SerTextOsdItemContainer* ItemContainer;
};

//SerChannelOsd not required because it only contains a string

struct SerProgrammeOsd
{
  int PresentTime;
  cxxtools::String PresentTitle;
  cxxtools::String PresentSubtitle;
  int FollowingTime;
  cxxtools::String FollowingTitle;
  cxxtools::String FollowingSubtitle;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerTextOsdItem& o);
void operator<<= (cxxtools::SerializationInfo& si, const SerTextOsd& o);
void operator<<= (cxxtools::SerializationInfo& si, const SerProgrammeOsd& o);

class TextOsdList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    TextOsdList(std::ostream* _out) { s = new StreamExtension(_out); total = 0; };
    ~TextOsdList() { delete s; }
    void setTotal(int _total) { total = _total; };
    virtual void printTextOsd(TextOsd* textOsd) { };
};

class XmlTextOsdList : TextOsdList
{
  public:
    XmlTextOsdList(std::ostream* _out) : TextOsdList(_out) { };
    ~XmlTextOsdList() { };
    virtual void printTextOsd(TextOsd* textOsd);
};

class JsonTextOsdList : TextOsdList
{
  public:
    JsonTextOsdList(std::ostream* _out) : TextOsdList(_out) { };
    ~JsonTextOsdList() { };
    virtual void printTextOsd(TextOsd* textOsd);
};

class HtmlTextOsdList : TextOsdList
{
  public:
    HtmlTextOsdList(std::ostream* _out) : TextOsdList(_out) { };
    ~HtmlTextOsdList() { };
    virtual void printTextOsd(TextOsd* textOsd);
};

class OsdWrapper
{
  protected:
    StreamExtension *s;
  public:
    OsdWrapper(std::ostream* _out) { s = new StreamExtension(_out); };
    ~OsdWrapper() { delete s; };
};

class ProgrammeOsdWrapper : OsdWrapper
{
  public:
    ProgrammeOsdWrapper(std::ostream* _out) : OsdWrapper(_out) { };
    ~ProgrammeOsdWrapper() { };
    void print(ProgrammeOsd* osd, std::string format);
    void printXml(ProgrammeOsd* osd);
    void printJson(ProgrammeOsd* osd);
    void printHtml(ProgrammeOsd* osd);
};

class ChannelOsdWrapper : OsdWrapper
{
  public:
    ChannelOsdWrapper(std::ostream* _out) : OsdWrapper(_out) { };
    ~ChannelOsdWrapper() { };
    void print(ChannelOsd* osd, std::string format);
    void printXml(ChannelOsd* osd);
    void printJson(ChannelOsd* osd);
    void printHtml(ChannelOsd* osd);
};

