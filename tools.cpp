#include "tools.h"

cxxtools::String UTF8Decode(std::string str)
{
  static cxxtools::Utf8Codec utf8;
  return utf8.decode(str);
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

std::string getRestParams(std::string service, std::string url)
{
  return url.substr(service.length(), url.length() - 1);
}

int getIntParam(std::string qparams, int level)
{
  std::string param = getStringParam(qparams, level);
  if ( param.length() > 0 )
  {
     return atoi(param.c_str());
  } 
  return -1;
}

std::string getStringParam(std::string qparams, int level)
{
  int start = -1;
  int end = -1;
  int on_level = 0;

  const char* params = qparams.c_str();

  for(int i=0;i<(int)qparams.length();i++)
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
          return qparams.substr(start + 1, end -1);
        }
        start = end;
        end = -1;
        on_level++;
      }
    }
  }
  return (std::string)"";
}

bool isFormat(std::string qparams, std::string format)
{
  int result = qparams.find(format);
  return result == -1 ? false : true;
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

