#include "wirbelscan.h"

#include <sstream>

using namespace std;
using namespace WIRBELSCAN_SERVICE;

void WirbelscanResponder::reply(ostream& out, cxxtools::http::Request& request,
		cxxtools::http::Reply& reply) {

	QueryHandler::addHeader(reply);

	if ( request.method() == "OPTIONS" ) {
	    reply.addHeader("Allow", "GET, POST, PUT");
	    reply.httpReturn(200, "OK");
	    return;
	}

	bool isGet = request.method() == "GET";
	bool isPut = request.method() == "PUT";
	bool isPost = request.method() == "POST";

	if ((wirbelscan = cPluginManager::GetPlugin("wirbelscan")) == NULL) {
		reply.httpReturn(403U, "wirbelscan was not found - pls install.");
		return;
	}

	if ( !isGet && !isPut && !isPost) {
		reply.httpReturn(403U, "To retrieve information use the GET method! To update Settings use the PUT method! ");
		return;
	}
	static cxxtools::Regex countriesRegex("/wirbelscan/countries.json");
	static cxxtools::Regex satellitesRegex("/wirbelscan/satellites.json");
	static cxxtools::Regex getStatusRegex("/wirbelscan/getStatus.json");
	static cxxtools::Regex getSetupRegex("/wirbelscan/getSetup.json");

	static cxxtools::Regex doCmdRegex("/wirbelscan/doCommand");
	static cxxtools::Regex setSetupRegex("/wirbelscan/setSetup");

	if (isGet && countriesRegex.match(request.url())) {
		replyCountries(out, request, reply);
	} else if (isGet && satellitesRegex.match(request.url())) {
		replySatellites(out, request, reply);
	} else if (isGet && getStatusRegex.match(request.url())) {
		replyGetStatus(out, request, reply);
	} else if (isPost && doCmdRegex.match(request.url())) {
		replyDoCmd(out, request, reply);
	} else if (isGet && getSetupRegex.match(request.url())) {
		replyGetSetup(out, request, reply);
	} else if (isPut && setSetupRegex.match(request.url())) {
		replySetSetup(out, request, reply);
	} else {
		replyGetStatus(out, request, reply);
	}
}

void WirbelscanResponder::replyCountries(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{

	QueryHandler q("/countries", request);

	CountryList* countryList;

	reply.addHeader("Content-Type", "application/json; charset=utf-8");
	countryList = (CountryList*) new JsonCountryList(&out);

	countryList->init();

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SCountry;

	cPreAllocBuffer countryBuffer;
	countryBuffer.size = 0;
	countryBuffer.count = 0;
	countryBuffer.buffer = NULL;

	wirbelscan->Service(cmd.str().c_str(), &countryBuffer); // query buffer size.

	SListItem *cbuf = (SListItem *) malloc(
			countryBuffer.size * sizeof(SListItem)); // now, allocate memory.
	countryBuffer.buffer = cbuf; // assign buffer
	wirbelscan->Service(cmd.str().c_str(), &countryBuffer); // fill buffer with values.

	for (unsigned int i = 0; i < countryBuffer.count; ++i)
	{
		countryList->addCountry(cbuf++);
	}

	countryList->setTotal(countryBuffer.count);
	countryList->finish();
	delete countryList;
}

void WirbelscanResponder::replyGetStatus(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
	QueryHandler q("/getStatus", request);

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SStatus;

	cWirbelscanStatus statusBuffer;

	wirbelscan->Service(cmd.str().c_str(), &statusBuffer); // query buffer size.

	StreamExtension se(&out);

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");

		cxxtools::JsonSerializer serializer(*se.getBasicStream());

		serializer.serialize((int)statusBuffer.status, "status");
		if (statusBuffer.status == StatusScanning)
		{
			serializer.serialize(statusBuffer.curr_device, "currentDevice");
			serializer.serialize((int) statusBuffer.progress, "progress");
			serializer.serialize((int) statusBuffer.strength, "strength");
			serializer.serialize(statusBuffer.transponder, "transponder");
			serializer.serialize((int) statusBuffer.numChannels, "numChannels");
			serializer.serialize((int) statusBuffer.newChannels, "newChannels");
			serializer.serialize((int) statusBuffer.nextTransponders,
					"nextTransponder");
		}
		serializer.finish();
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void WirbelscanResponder::replyGetSetup(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
	QueryHandler q("/getSetup", request);

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SSetup;

	cWirbelscanScanSetup setupBuffer;

	wirbelscan->Service(cmd.str().c_str(), &setupBuffer); // query buffer size.

	StreamExtension se(&out);

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");

		cxxtools::JsonSerializer serializer(*se.getBasicStream());

		serializer.serialize((int) setupBuffer.verbosity, "verbosity");
		serializer.serialize((int) setupBuffer.logFile, "logFile");
		serializer.serialize((int) setupBuffer.DVB_Type, "DVB_Type");
		serializer.serialize((int) setupBuffer.DVBT_Inversion,
				"DVBT_Inversion");
		serializer.serialize((int) setupBuffer.DVBC_Inversion,
				"DVBC_Inversion");
		serializer.serialize((int) setupBuffer.DVBC_Symbolrate,
				"DVBC_Symbolrate");
		serializer.serialize((int) setupBuffer.DVBC_QAM, "DVBC_QAM");
		serializer.serialize((int) setupBuffer.CountryId, "CountryId");
		serializer.serialize((int) setupBuffer.SatId, "SatId");
		serializer.serialize((int) setupBuffer.scanflags, "scanflags");
		serializer.serialize((int) setupBuffer.ATSC_type, "ATSC_type");

		serializer.finish();
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void WirbelscanResponder::replySetSetup(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
	QueryHandler q("/setSetup", request);

	std::stringstream getcmd;
	getcmd << SPlugin << "Get" << SSetup;

	cWirbelscanScanSetup setupBuffer;

	wirbelscan->Service(getcmd.str().c_str(), &setupBuffer); // query buffer size.

	// deactivated some sanity checks until it's clear which values are valid

	if (q.hasJson("verbosity"))
	{
	    int verbosity = q.getBodyAsInt("verbosity");
	    if (verbosity >= 0 && verbosity < 6) {
		setupBuffer.verbosity = verbosity;
	    }
	}

	if (q.hasJson("logFile"))
	{
	    int logFile = q.getBodyAsInt("logFile");
	    if (logFile >= 0 && logFile < 3) {
		setupBuffer.logFile = logFile;
	    }
	}

	if (q.hasJson("DVB_Type"))
	{
	    int dvbType = q.getBodyAsInt("DVB_Type");
	    //if ((dvbType >= 0 && dvbType < 6) || dvbType == 999) {
		setupBuffer.DVB_Type = dvbType;
	    //}
	}

	if (q.hasJson("DVBT_Inversion"))
	{
	    int dvbtInversion = q.getBodyAsInt("DVBT_Inversion");
	    if (dvbtInversion >= 0 && dvbtInversion < 2) {
		setupBuffer.DVBT_Inversion = dvbtInversion;
	    }
	}

	if (q.hasJson("DVBC_Inversion"))
	{
	    int dvbcInversion = q.getBodyAsInt("DVBC_Inversion");
	    if (dvbcInversion >= 0 && dvbcInversion < 2) {
		setupBuffer.DVBC_Inversion = dvbcInversion;
	    }
	}

	if (q.hasJson("DVBC_Symbolrate"))
	{
	    int dvbcSymbolrate = q.getBodyAsInt("DVBC_Symbolrate");
	    //if (dvbcSymbolrate >= 0 && dvbcSymbolrate < 16) {
		setupBuffer.DVBC_Symbolrate = dvbcSymbolrate;
	    //}
	}

	if (q.hasJson("DVBC_QAM"))
	{
	    int dvbcQam = q.getBodyAsInt("DVBC_QAM");
	    //if (dvbcQam >= 0 && dvbcQam < 5) {
		setupBuffer.DVBC_QAM = dvbcQam;
	    //}
	}

	if (q.hasJson("CountryId"))
	{
		setupBuffer.CountryId = q.getBodyAsInt("CountryId");
	}

	if (q.hasJson("SatId"))
	{
		setupBuffer.SatId = q.getBodyAsInt("SatId");
	}

	if (q.hasJson("scanflags"))
	{
	    int scanFlags = q.getBodyAsInt("scanflags");
	    if (scanFlags >= 0 && scanFlags < 32) {
		setupBuffer.scanflags = scanFlags;
	    }
	}

	if (q.hasJson("ATSC_type"))
	{
	    int atscType = q.getBodyAsInt("ATSC_type");
	    //if (atscType >= 0 && atscType < 3) {
		setupBuffer.ATSC_type = atscType;
	    //}
	}

	std::stringstream setcmd;
	setcmd << SPlugin << "Set" << SSetup;
	wirbelscan->Service(setcmd.str().c_str(), &setupBuffer); // query buffer size.

	if (q.isFormat(".json"))
	{
		replyGetSetup(out, request, reply);
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void WirbelscanResponder::replyDoCmd(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
	QueryHandler q("/doCommand", request);

	if ( !q.hasBody("command") || q.getBodyAsInt("command") < 0 || q.getBodyAsInt("command") > 2) {
	    reply.httpReturn(400, "command body parameter is missing!");
	    return;
	}

	cWirbelscanCmd commandBuffer;

	commandBuffer.cmd = (s_cmd)q.getBodyAsInt("command");

	std::stringstream cmd;
	cmd << SPlugin << SCommand;

	wirbelscan->Service(cmd.str().c_str(), &commandBuffer); // query buffer size.

	StreamExtension se(&out);

	if (q.isFormat(".json"))
	{
		reply.addHeader("Content-Type", "application/json; charset=utf-8");

		cxxtools::JsonSerializer serializer(*se.getBasicStream());

		serializer.serialize(commandBuffer.replycode, "replycode");
		serializer.finish();
	}
	else
	{
		reply.httpReturn(403,
				"Resources are not available for the selected format. (Use: .json)");
		return;
	}
}

void operator<<= (cxxtools::SerializationInfo& si, const SerListItem& l)
{
  si.addMember("id") <<= l.id;
  si.addMember("shortName") <<= l.shortName;
  si.addMember("fullName") <<= l.fullName;
}

CountryList::CountryList(ostream* _out)
{
  s = new StreamExtension(_out);
}

CountryList::~CountryList()
{
  delete s;
}

void JsonCountryList::addCountry(SListItem* country)
{
  SerListItem serCountry;

  serCountry.id = country->id;
  serCountry.shortName = StringExtension::UTF8Decode(country->short_name);
  serCountry.fullName = StringExtension::UTF8Decode(country->full_name);

  SerCountries.push_back(serCountry);
}

void JsonCountryList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(SerCountries, "countries");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}

void WirbelscanResponder::replySatellites(ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{

	QueryHandler q("/satellites", request);

	SatelliteList* satelliteList;

	reply.addHeader("Content-Type", "application/json; charset=utf-8");
	satelliteList = (SatelliteList*) new JsonSatelliteList(&out);

	satelliteList->init();

	std::stringstream cmd;
	cmd << SPlugin << "Get" << SSat;

	cPreAllocBuffer satelliteBuffer;
	satelliteBuffer.size = 0;
	satelliteBuffer.count = 0;
	satelliteBuffer.buffer = NULL;

	wirbelscan->Service(cmd.str().c_str(), &satelliteBuffer); // query buffer size.

	SListItem *cbuf = (SListItem *) malloc(
			satelliteBuffer.size * sizeof(SListItem)); // now, allocate memory.
	satelliteBuffer.buffer = cbuf; // assign buffer
	wirbelscan->Service(cmd.str().c_str(), &satelliteBuffer); // fill buffer with values.

	for (unsigned int i = 0; i < satelliteBuffer.count; ++i)
	{
		satelliteList->addSatellite(cbuf++);
	}

	satelliteList->setTotal(satelliteBuffer.count);
	satelliteList->finish();
	delete satelliteList;
}

SatelliteList::SatelliteList(ostream* _out)
{
  s = new StreamExtension(_out);
}

SatelliteList::~SatelliteList()
{
  delete s;
}

void JsonSatelliteList::addSatellite(SListItem* satellite)
{
  SerListItem serSatellite;

  serSatellite.id = satellite->id;
  serSatellite.shortName = StringExtension::UTF8Decode(satellite->short_name);
  serSatellite.fullName = StringExtension::UTF8Decode(satellite->full_name);

  SerSatellites.push_back(serSatellite);
}

void JsonSatelliteList::finish()
{
  cxxtools::JsonSerializer serializer(*s->getBasicStream());
  serializer.serialize(SerSatellites, "satellites");
  serializer.serialize(Count(), "count");
  serializer.serialize(total, "total");
  serializer.finish();
}
