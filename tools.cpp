#include "tools.h"

// --- StreamExtension ---------------------------------------------------------

StreamExtension::StreamExtension(std::ostream *out)
{
  _out = out;
}

std::ostream* StreamExtension::getBasicStream()
{
  return _out;
}

void StreamExtension::write(std::string str)
{
  _out->write(str.c_str(), str.length());
}

void StreamExtension::writeHtmlHeader()
{
  write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
  write("<html xml:lang=\"en\" lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  write("<head>\n");
  write("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
  write("</head><body>");
}

void StreamExtension::writeXmlHeader()
{
  write("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
}

// --- VdrExtension -----------------------------------------------------------

cChannel* VdrExtension::getChannel(int number)
{
  if( number == -1 || number >= Channels.Count() ) { return NULL; }

  cChannel* result = NULL;
  int counter = 1;
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
  {
      if (!channel->GroupSep()) {
         if (counter == number)
         {
            result = channel;
            break;
         }
         counter++;
      }
  }
  return result;
}

int VdrExtension::scanForFiles(const std::string wildcardpath, std::vector< std::string >& files)
{
  int found = 0;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  if ( wildcardpath.empty() == false && glob(wildcardpath.c_str(), GLOB_DOOFFS, NULL, &globbuf) == 0) {
     for (size_t i = 0; i < globbuf.gl_pathc; i++) {
         std::string file(globbuf.gl_pathv[i]);

         size_t delimPos = file.find_last_of('/');
         files.push_back(file.substr(delimPos+1));
         found++;
     }
     globfree(&globbuf);
  }
  return found;
}

// --- StringExtension --------------------------------------------------------

std::string StringExtension::itostr(int i)
{
  std::stringstream str;
  str << i;
  return str.str();
}

int StringExtension::strtoi(std::string str)
{
  static cxxtools::Regex regex("[0-9]{1,}");
  if(!regex.match(str)) return -LOWINT; // lowest possible integer
  return atoi(str.c_str());
}

std::string StringExtension::replace(std::string const& text, std::string const& substring, std::string const& replacement)
{
  std::string result = text;
  std::string::size_type pos = 0;
  while ( ( pos = result.find( substring, pos ) ) != std::string::npos ) {
    result.replace( pos, substring.length(), replacement );
    pos += replacement.length();
  }
  return result;
}

std::string StringExtension::encodeToXml(const std::string &str)
{
    //source: http://www.mdawson.net/misc/xmlescape.php
    std::ostringstream result;

    for( std::string::const_iterator iter = str.begin(); iter!=str.end(); iter++ )
    {
         unsigned char c = (unsigned char)*iter;

         switch( c )
         {
             case '&': result << "&amp;"; break;
             case '<': result << "&lt;"; break;
             case '>': result << "&gt;"; break;
             case '"': result << "&quot;"; break;
             case '\'': result << "&apos;"; break;

             default:
                   result << c;
         }
    }

    std::string res = result.str();
    std::string converted;
    utf8::replace_invalid(res.begin(), res.end(), back_inserter(converted));
    return converted;
}

cxxtools::String StringExtension::UTF8Decode(std::string str)
{
  static cxxtools::Utf8Codec utf8;
  std::string temp;
  utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp));
  return utf8.decode(temp);
}

// --- QueryHandler -----------------------------------------------------------

QueryHandler::QueryHandler(std::string service, cxxtools::http::Request& request)
{
  _url = request.url();
  _service = service;
  _options.parse_url(request.qparams());

  std::string params = _url.substr(_service.length(), _url.length() - 1);
  parseRestParams(params);
}

QueryHandler::~QueryHandler()
{

}

void QueryHandler::parseRestParams(std::string params)
{
  int start = -1;

  for(int i=0;i<(int)params.length();i++)
  {
    if(params[i] == '/')
    {
      if(start == -1)
      {
        start = i;
      } else {
        std::string p = params.substr(start+1, (i-1)-(start));
        _params.push_back(p);
        start = i;
      }
    }
  }

  if(start != (int)params.length() - 1) {
    _params.push_back(params.substr(start + 1, params.length() - 1));
  }
}

std::string QueryHandler::getParamAsString(int level)
{
  if ( level >= (int)_params.size() )
     return "";
  return _params[level];
}

std::string QueryHandler::getOptionAsString(std::string name)
{
  return _options.param(name);
}

int QueryHandler::getParamAsInt(int level)
{
  return StringExtension::strtoi(getParamAsString(level));  
}

int QueryHandler::getOptionAsInt(std::string name)
{
  return StringExtension::strtoi(getOptionAsString(name));
}

bool QueryHandler::isFormat(std::string format)
{
  if ((int)_url.find(format) != -1)
     return true;
  return false;
}

// --- HtmlRequestParser ------------------------------------------------------

HtmlRequestParser::HtmlRequestParser(cxxtools::http::Request& request)
{
  query.parse_url(request.bodyStr());
}

HtmlRequestParser::~HtmlRequestParser()
{

}

std::string HtmlRequestParser::getValueAsString(std::string name)
{
  return query.param(name);
}

int HtmlRequestParser::getValueAsInt(std::string name)
{
  return StringExtension::strtoi(getValueAsString(name));
}
