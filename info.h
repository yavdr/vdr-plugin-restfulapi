#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include "tools.h"
#include <time.h>
#include <vector>
#include "statusmonitor.h"
#include <sys/ioctl.h>

#define FRONTEND_DEVICE "/dev/dvb/adapter%d/frontend%d"

struct SerService
{
  cxxtools::String Path;
  int Version;
  bool Internal;
};

struct SerPlugin
{
  cxxtools::String Name;
  cxxtools::String Version;
};

struct SerVDR
{
  std::string version;
  std::vector< struct SerPlugin > plugins;
  std::vector< struct SerDevice > devices;
};

struct SerPlayerInfo
{
  cxxtools::String Name;
  cxxtools::String FileName;
};

struct SerDiskSpaceInfo
{
  int FreeMB;
  int UsedPercent;
  int FreeMinutes;
  std::string Description;
};

struct SerDevice {
  cxxtools::String Name;
  bool dvbc;
  bool dvbs;
  bool dvbt;
  bool atsc;
  bool primary;
  bool hasDecoder;

  bool HasCi;
  int SignalStrength;
  int SignalQuality;
  uint16_t str;
  uint16_t snr;
  uint32_t ber;
  uint32_t unc;
  cxxtools::String status;
  int Adapter;
  int Frontend;
  cxxtools::String Type;

  int Number;
  cxxtools::String ChannelId;
  cxxtools::String ChannelName;
  int ChannelNr;
  bool Live;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerService& s);
void operator<<= (cxxtools::SerializationInfo& si, const SerPlugin& p);
void operator<<= (cxxtools::SerializationInfo& si, const SerVDR& vdr);
void operator<<= (cxxtools::SerializationInfo& si, const SerPlayerInfo& pi);
void operator<<= (cxxtools::SerializationInfo& si, const SerDiskSpaceInfo& ds);
void operator<<= (cxxtools::SerializationInfo& si, const SerDevice& d);

class InfoResponder : public cxxtools::http::Responder
{
private:
  SerDevice getDeviceSerializeInfo(int index);
  public:
    explicit InfoResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service) { };
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
    virtual void replyHtml(StreamExtension& se);
    virtual void replyJson(StreamExtension& se);
    virtual void replyXml(StreamExtension& se);
};

typedef cxxtools::http::CachedService<InfoResponder> InfoService;

