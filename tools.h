#include <glob.h>
#include <list>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <cxxtools/regex.h>
#include <cxxtools/string.h>
#include <cxxtools/utf8codec.h>
#include <vdr/channels.h>
#include "utf8_checked.h"

// General Helper Methods
cxxtools::String UTF8Decode(std::string str);
std::string UTF8Encode(cxxtools::String str);
void write(std::ostream* out, std::string str);
void writeHtmlHeader(std::ostream* out);
std::string encodeToXml( const std::string &sSrc );

int scanForFiles(const std::string wildcardpath, std::vector< std::string >& images);
std::string itostr(int i);
std::string replace(std::string const& text, std::string const& substring, std::string const& replacement);

// Request Helper Methods
std::string getRestParams(std::string service, std::string url);
int getIntParam(std::string qparams, int level);
std::string getStringParam(std::string qparams, int level);
bool isFormat(std::string qparams, std::string format);

// VDR API Helper Methods
cChannel* getChannel(int number);
