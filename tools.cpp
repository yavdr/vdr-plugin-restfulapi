#include "tools.h"

int GetIntParam(std::string qparams, std::string param)
{
  int result = -1;
  int begin_parse = qparams.find(param);
  if ( begin_parse != -1 )
  {
     int end_parse = qparams.find_first_of("& ", begin_parse + param.length());
     if ( end_parse == -1 ) { end_parse = qparams.length() - 1; }
     if( qparams.length() > 0 && begin_parse != -1 ) { result = atoi(qparams.substr(begin_parse+param.length(), end_parse).c_str()); }
  }
  return result;
}

cChannel* GetChannel(int number)
{
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
