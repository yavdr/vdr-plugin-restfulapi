#include <glob.h>
#include <list>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <cxxtools/regex.h>
#include <cxxtools/string.h>
#include <cxxtools/utf8codec.h>
#include <cxxtools/http/request.h>
#include <cxxtools/query_params.h>
#include <cxxtools/regex.h>
#include <vdr/channels.h>
#include <vdr/timers.h>
#include "utf8_checked.h"

#define LOWINT 2147483648;

#ifndef RESTFULAPI_EXTENSIONS
#define RESTFULAPI_EXTENSIONS

class StreamExtension
{
  private:
    std::ostream *_out;
  public:
    StreamExtension(std::ostream *out);
    ~StreamExtension() { };
    std::ostream* getBasicStream();
    void write(std::string str);
    void writeHtmlHeader();
    void writeXmlHeader();
    bool writeBinary(std::string path);
};

class FileCaches
{
  private:
    std::vector< std::string > eventImages;
    std::vector< std::string > channelLogos;
  public:
    FileCaches() {
         cacheEventImages();
         cacheChannelLogos();
      };
    ~FileCaches() { };
    static FileCaches* get();
    void cacheEventImages();
    void cacheChannelLogos();
    int searchEventImage(cEvent* event, std::vector< std::string >& files);
    std::string searchChannelLogo(cChannel *channel);
};

class VdrExtension
{
  public:
    static cChannel* getChannel(int number);
    static cChannel* getChannel(std::string id);
    static cTimer* getTimer(std::string id);
    static std::string getTimerID(cTimer* timer);
    static int scanForFiles(const std::string wildcardpath, std::vector< std::string >& files);
    static bool doesFileExistInFolder(std::string wildcardpath, std::string filename);
    static bool IsRadio(cChannel* channel);
};

class StringExtension
{
  public:
    static std::string itostr(int i);
    static int strtoi(std::string str);
    static std::string replace(std::string const& text, std::string const& substring, std::string const& replacement);
    static std::string encodeToXml(const std::string &str);
    static cxxtools::String UTF8Decode(std::string str);
    static std::string toLowerCase(std::string str);
};

class QueryHandler
{
  private:
    std::string _url;
    std::string _service;
    std::vector< std::string > _params;
    cxxtools::QueryParams _options;
    void parseRestParams(std::string params);
    std::string _format;
  public:
    QueryHandler(std::string service, cxxtools::http::Request& request);
    ~QueryHandler();
    std::string getParamAsString(int level);
    std::string getOptionAsString(std::string name);
    int getParamAsInt(int level);
    int getOptionAsInt(std::string name);
    bool isFormat(std::string format);
    std::string getFormat() { return _format; }
};

class HtmlRequestParser
{
  private:
    cxxtools::QueryParams query;
  public:
    HtmlRequestParser(cxxtools::http::Request& request);
    ~HtmlRequestParser();
    std::string getValueAsString(std::string name);
    int getValueAsInt(std::string name);
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
    ~BaseList() { };
    virtual void activateLimit(int _start, int _limit);
    virtual bool filtered();
    virtual int Count() { return counter; }
};
#endif

