#include "tools.h"

std::string UTF8Encode(cxxtools::String str)
{
  static cxxtools::Utf8Codec utf8;
  return utf8.encode(str);
}

cxxtools::String UTF8Decode(std::string str)
{
  static cxxtools::Utf8Codec utf8;
  std::string temp;
  utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp));
  return utf8.decode(temp);
}

void write(std::ostream* out, std::string str)
{
  out->write(str.c_str(), str.length());
}

void writeHtmlHeader(std::ostream* out)
{
  write(out, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  write(out, "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
  write(out, "<html xml:lang=\"en\" lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  write(out, "<head>\n");
  write(out, "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
  write(out, "</head><body>");
}

std::string encodeToXml( const std::string &sSrc )
{
    //source: http://www.mdawson.net/misc/xmlescape.php
    std::ostringstream sRet;

    for( std::string::const_iterator iter = sSrc.begin(); iter!=sSrc.end(); iter++ )
    {
         unsigned char c = (unsigned char)*iter;

         switch( c )
         {
             case '&': sRet << "&amp;"; break;
             case '<': sRet << "&lt;"; break;
             case '>': sRet << "&gt;"; break;
             case '"': sRet << "&quot;"; break;
             case '\'': sRet << "&apos;"; break;

             default:
                   sRet << c;
         }
    }

    std::string res = sRet.str();
    std::string converted;
    utf8::replace_invalid(res.begin(), res.end(), back_inserter(converted));
    return converted;
}

std::string getRestParams(std::string service, std::string url)
{
  return url.substr(service.length(), url.length() - 1);
}

int getIntParam(std::string qparams, int level)
{
  std::string param = getStringParam(qparams, level);
  if ( param.length() > 0 )
  {
     int res = atoi(param.c_str());
     if ( res == 0 ) {
        return (int)param.find_first_of("0") > -1 ? res : -1;
     } else {
        return res;
     }
  } 
  return -1;
}

std::string getStringParam(std::string params, int level)
{
  int start = -1;
  int end = -1;
  int on_level = 0;

  for(int i=0;i<(int)params.length();i++)
  {
    if(params[i] == '/')
    {
      if(start == -1)
      {
        start = i;
      } else {
        end = i;
        if(on_level == level)
        {
          return params.substr(start + 1, end -1);
        }
        start = end;
        end = -1;
        on_level++;
      }
    }
  }
  if(start != -1 && on_level == level) {
    return params.substr(start + 1, params.length() - 1);
  }
  return (std::string)"";
}

bool isFormat(std::string qparams, std::string format)
{
  int result = qparams.find(format);
  return result == -1 ? false : true;
}

int scanForFiles(const std::string wildcardpath, std::vector< std::string >& images)
{
  int found = 0;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  if ( wildcardpath.empty() == false && glob(wildcardpath.c_str(), GLOB_DOOFFS, NULL, &globbuf) == 0) {
     for (size_t i = 0; i < globbuf.gl_pathc; i++) {
         std::string imagefile(globbuf.gl_pathv[i]);
         esyslog("restfulapi: imagefile:/%s/", imagefile.c_str());
         size_t delimPos = imagefile.find_last_of('/');
         images.push_back(imagefile.substr(delimPos+1));
         found++;
     }
     globfree(&globbuf);   
  }
  return found;
}

std::string itostr(int i)
{
  std::stringstream str;
  str << i;
  return str.str();  
}

cChannel* getChannel(int number)
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

