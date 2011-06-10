#include <glob.h>
#include <list>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
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

void write(std::ostream* out, std::string str);
void writeHtmlHeader(std::ostream* out);

#ifndef RESTFULAPI_EXTENSIONS
#define RESTFULAPI_EXTENSIONS

class VdrExtension
{
  public:
    static cChannel* getChannel(int number);
    static int scanForFiles(const std::string wildcardpath, std::vector< std::string >& files);
};

class StringExtension
{
  public:
    static std::string itostr(int i);
    static int strtoi(std::string str);
    static std::string replace(std::string const& text, std::string const& substring, std::string const& replacement);
    static std::string encodeToXml(const std::string &str);
    static cxxtools::String UTF8Decode(std::string str);
};

class QueryHandler
{
  private:
    std::string _url;
    std::string _service;
    std::vector< std::string > _params;
    cxxtools::QueryParams _options;
    void parseRestParams(std::string params);
  public:
    QueryHandler(std::string service, cxxtools::http::Request& request);
    ~QueryHandler();
    std::string getParamAsString(int level);
    std::string getOptionAsString(std::string name);
    int getParamAsInt(int level);
    int getOptionAsInt(std::string name);
    bool isFormat(std::string format);
};
#endif
