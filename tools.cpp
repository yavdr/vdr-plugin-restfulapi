#include "tools.h"

int GetIntParam(std::string qparams, int level)
{
  int start = -1;
  int end = -1;
  int on_level = 0;

  const char* params = qparams.c_str();

  for(int i=0;i<qparams.length();i++)
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
          return atoi(qparams.substr(start + 1, end -1).c_str());
        }
        start = end;
        end = -1;
        on_level++;
      }
    }
  }
  return -1;

  /* old html param parser
 
  int result = -1;
  int begin_parse = qparams.find(param);
  if ( begin_parse != -1 )
  {
     int end_parse = qparams.find_first_of("&/ ", begin_parse + param.length());
     if ( end_parse == -1 ) { end_parse = qparams.length() - 1; }
     if( qparams.length() > 0 && begin_parse != -1 ) { result = atoi(qparams.substr(begin_parse+param.length(), end_parse).c_str()); }
  }
  return result;*/
}

bool IsFormat(std::string qparams, std::string format)
{
  int result = qparams.find(format);
  return result == -1 ? false : true;
}

cChannel* GetChannel(int number)
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

