#include <vector>
#include <ostream>

#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <cxxtools/jsonserializer.h>
#include <cxxtools/serializationinfo.h>
#include <vdr/osdbase.h>
#include <vdr/plugin.h>
#include "tools.h"

#include "wirbelscan/wirbelscan_services.h"

using namespace WIRBELSCAN_SERVICE;

class WirbelscanResponder: public cxxtools::http::Responder {
public:
	explicit WirbelscanResponder(cxxtools::http::Service& service) :
			cxxtools::http::Responder(service) {};

	virtual ~WirbelscanResponder() {};

	virtual void reply(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

	virtual void replyCountries(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

	virtual void replySatellites(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

	virtual void replyGetStatus(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

	virtual void replyGetSetup(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

	virtual void replySetSetup(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

	virtual void replyDoCmd(std::ostream& out, cxxtools::http::Request& request,
			cxxtools::http::Reply& reply);

private:
	cPlugin *wirbelscan;
};

typedef cxxtools::http::CachedService<WirbelscanResponder> WirbelscanService;

struct SerListItem
{
  int id;
  cxxtools::String shortName;
  cxxtools::String fullName;
};

void operator<<= (cxxtools::SerializationInfo& si, const SerListItem& l);

class CountryList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    explicit CountryList(std::ostream* _out);
    virtual ~CountryList();
    virtual void init() { };
    virtual void addCountry(SListItem* country) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class JsonCountryList : CountryList
{
  private:
    std::vector < struct SerListItem > SerCountries;
  public:
    explicit JsonCountryList(std::ostream* _out) : CountryList(_out) { };
    ~JsonCountryList() { };
    virtual void addCountry(SListItem* country);
    virtual void finish();
};

class SatelliteList : public BaseList
{
  protected:
    StreamExtension *s;
    int total;
  public:
    explicit SatelliteList(std::ostream* _out);
    virtual ~SatelliteList();
    virtual void init() { };
    virtual void addSatellite(SListItem* satellite) { };
    virtual void finish() { };
    virtual void setTotal(int _total) { total = _total; }
};

class JsonSatelliteList : SatelliteList
{
  private:
    std::vector < struct SerListItem > SerSatellites;
  public:
    explicit JsonSatelliteList(std::ostream* _out) : SatelliteList(_out) { };
    ~JsonSatelliteList() { };
    virtual void addSatellite(SListItem* satellite);
    virtual void finish();
};
