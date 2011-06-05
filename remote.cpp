#include "remote.h"

void RemoteResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if (request.method() != "PUT") {
     reply.httpReturn(403, "Only PUT method is support by the remote control");
     return;
  }

  std::string params = getRestParams((std::string)"/remote", request.url());
  std::string key = getStringParam(params, 0);

  if (key.length() == 0) {
     reply.httpReturn(404, "Please add a key to the parameter list, see API-file for more details.");
     return;
  }

  if (!keyPairList->hitKey(key.c_str())) {
     reply.httpReturn(404, "Remote Control does not support the requested key.");
  }
}

KeyPairList::KeyPairList()
{
  append( "kup" , 		kUp 		);
  append( "kdown" , 		kDown 		);
  append( "kmenu" , 		kMenu 		);
  append( "kok" , 		kOk 		);
  append( "kback" , 		kBack 		);
  append( "kleft" , 		kLeft 		);
  append( "kright" , 		kRight 		);
  append( "kred" , 		kRed 		);
  append( "kgreen" , 		kGreen 		);
  append( "kyellow" , 		kYellow 	);
  append( "kblue" , 		kBlue 		);
  append( "k0" , 		k0 		);
  append( "k1" , 		k1 		);
  append( "k2" , 		k2 		);
  append( "k3" , 		k3 		);
  append( "k4" , 		k4 		);
  append( "k5" , 		k5 		);
  append( "k6" , 		k6 		);
  append( "k7" , 		k7 		);
  append( "k8" , 		k8 		);
  append( "k9" , 		k9 		);
  append( "kinfo" , 		kInfo 		);
  append( "kplay" , 		kPlay 		);
  append( "kpause" , 		kPause 		);
  append( "kstop" , 		kStop 		);
  append( "krecord" , 		kRecord 	);
  append( "kfastfwd" , 		kFastFwd 	);
  append( "kfastrew" , 		kFastRew 	);
  append( "knext" , 		kNext 		);
  append( "kprev" , 		kPrev 		);
  append( "kpower" , 		kPower 		);
  append( "kchanup" , 		kChanUp 	);
  append( "kchandn" , 		kChanDn 	);
  append( "kvolup" , 		kVolUp 		);
  append( "kvoldn" , 		kVolDn 		);
  append( "kmute" , 		kMute 		);
  append( "kaudio" , 		kAudio 		);
  append( "ksubtitles" , 	kSubtitles 	);
  append( "kschedule" ,		kSchedule 	);
  append( "kchannels" ,		kChannels 	);
  append( "ktimers" , 		kTimers 	);
  append( "krecordings" , 	kRecordings 	);
  append( "ksetup" , 		kSetup 		);
  append( "kcommands" ,		kCommands 	);
  append( "kuser0" , 		kUser0 		);
  append( "kuser1" , 		kUser1 		);
  append( "kuser2" , 		kUser2 		);
  append( "kuser3" , 		kUser3 		);
  append( "kuser4" , 		kUser4 		);
  append( "kuser5" , 		kUser5 		);
  append( "kuser6" , 		kUser6 		);
  append( "kuser7" , 		kUser7 		);
  append( "kuser8" , 		kUser8 		);
  append( "kuser9" , 		kUser9 		);
  append( "knone" , 		kNone 		);
  append( "kkbd" , 		kKbd 		);
}

KeyPairList::~KeyPairList()
{

}

bool KeyPairList::hitKey(std::string key)
{ 
  for (int i=0;i<(int)key.length();i++) {
    key[i] = std::tolower(key[i]);
  }
  
  for (int i=0;i<(int)keys.size();i++)
  {
    if (std::string(keys[i].str) == key) {
       cRemote::Put(keys[i].key);
       return true;
    }
  }
  return false;
}

void KeyPairList::append(const char* str, eKeys key)
{
  eKeyPair keyPair = { str, key };
  keys.push_back(keyPair);
}
