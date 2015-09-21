#include "remote.h"
using namespace std;

void RemoteResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "POST");
      reply.httpReturn(200, "OK");
      return;
  }

  if (request.method() != "POST") {
     reply.httpReturn(403, "Only POST method is support by the remote control");
     return;
  }

  if ( (int)request.url().find("/remote/switch") != -1 ) {
     QueryHandler q("/remote/switch", request);
     const cChannel* channel = VdrExtension::getChannel(q.getParamAsString(0));
     if ( channel == NULL ) {
        reply.httpReturn(404, "Channel-Id is not valid.");
     } else {
        TaskScheduler::get()->SwitchableChannel(channel->GetChannelID());
     }

     return;
  } 

  if (!keyPairList->hitKey(request, reply)) {
     reply.httpReturn(404, "Remote Control does not support the requested key.");
  }
}

KeyPairList::KeyPairList()
{
  append( "up" ,         kUp           );
  append( "down" ,       kDown         );
  append( "menu" ,       kMenu         );
  append( "ok" ,         kOk           );
  append( "back" ,       kBack         );
  append( "left" ,       kLeft         );
  append( "right" ,      kRight        );
  append( "red" ,        kRed          );
  append( "green" ,      kGreen        );
  append( "yellow" ,     kYellow       );
  append( "blue" ,       kBlue         );
  append( "0" ,          k0            );
  append( "1" ,          k1            );
  append( "2" ,          k2            );
  append( "3" ,          k3            );
  append( "4" ,          k4            );
  append( "5" ,          k5            );
  append( "6" ,          k6            );
  append( "7" ,          k7            );
  append( "8" ,          k8            );
  append( "9" ,          k9            );
  append( "info" ,       kInfo         );
  append( "play" ,       kPlay         );
  append( "pause" ,      kPause        );
  append( "stop" ,       kStop         );
  append( "record" ,     kRecord       );
  append( "fastfwd" ,    kFastFwd      );
  append( "fastrew" ,    kFastRew      );
  append( "next" ,       kNext         );
  append( "prev" ,       kPrev         );
  append( "power" ,      kPower        );
  append( "chanup" ,     kChanUp       );
  append( "chandn" ,     kChanDn       );
  append( "chanprev",    kChanPrev     );
  append( "volup" ,      kVolUp        );
  append( "voldn" ,      kVolDn        );
  append( "mute" ,       kMute         );
  append( "audio" ,      kAudio        );
  append( "subtitles" ,  kSubtitles    );
  append( "schedule" ,   kSchedule     );
  append( "channels" ,   kChannels     );
  append( "timers" ,     kTimers       );
  append( "recordings",  kRecordings   );
  append( "setup" ,      kSetup        );
  append( "commands" ,   kCommands     );
  append( "user0" ,      kUser0        );
  append( "user1" ,      kUser1        );
  append( "user2" ,      kUser2        );
  append( "user3" ,      kUser3        );
  append( "user4" ,      kUser4        );
  append( "user5" ,      kUser5        );
  append( "user6" ,      kUser6        );
  append( "user7" ,      kUser7        );
  append( "user8" ,      kUser8        );
  append( "user9" ,      kUser9        );
  append( "none" ,       kNone         );
  append( "kbd" ,        kKbd          );
}

KeyPairList::~KeyPairList()
{

}

bool KeyPairList::hitKey(cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{ 
  
  if ( (int)request.url().find("/remote/kbd") != -1) {
    QueryHandler q("/remote/kbd", request);
    cxxtools::String kbd = StringExtension::UTF8Decode(q.getBodyAsString("kbd"));
    if ( kbd == StringExtension::UTF8Decode("") ) {
	reply.httpReturn(400, "Key is empty.");
    }
    std::size_t n = 0;
    while (kbd[n]) {
      cRemote::Put(KBDKEY(kbd[n]));
      ++n;
    }
    return true;

  } else if ( (int)request.url().find("/remote/seq") != -1) {
    QueryHandler q("/remote/seq", request);
    JsonArray* seq = q.getBodyAsArray("seq");

    if ( seq == NULL ) {
	reply.httpReturn(400, "Sequence is empty.");
        return false;
    }

    for (int i = 0; i < seq->CountItem(); i++) {
      JsonBase* jsonBase = seq->GetItem(i);
      if (jsonBase->IsBasicValue()) {
	JsonBasicValue* jsonBasicValue = (JsonBasicValue*)jsonBase;
	if (jsonBasicValue->IsString()) {
          string key = jsonBasicValue->ValueAsString();
          for (int n=0;n<(int)key.length();n++ ) {
            key[n] = tolower(key[n]);
          }
	  for (int x=0;x<(int)keys.size();x++) {
	    if (string(keys[x].str) == key) {
		cRemote::Put(keys[x].key);
	    }
	  }
	}
      }
    }

    return true;

  } else {
    QueryHandler q("/remote", request);
    string key = q.getParamAsString(0);

    if (key.length() == 0) {
       reply.httpReturn(404, "Please add a key to the parameter list, see API-file for more details.");
       return false;
    }

    for (int i=0;i<(int)key.length();i++) {
      key[i] = tolower(key[i]);
    }

    for (int i=0;i<(int)keys.size();i++)
    {
      if (string(keys[i].str) == key) {
        cRemote::Put(keys[i].key);
        return true;
      }
    }
  }



  return false;
}

void KeyPairList::append(const char* str, eKeys key)
{
  eKeyPair keyPair = { str, key };
  keys.push_back(keyPair);
}
