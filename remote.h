#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include "tools.h"
#include <vector>

#include <vdr/channels.h>
#include <vdr/keys.h>
#include <vdr/remote.h>

struct eKeyPair
{
  const char* str;
  eKeys key;
};

class KeyPairList
{
  private:
    std::vector< struct eKeyPair > keys;
    void append(const char* str, eKeys key);
  public:
    KeyPairList();
    ~KeyPairList();
    bool hitKey(std::string key);
};

class RemoteResponder : public cxxtools::http::Responder
{
  private:
    KeyPairList* keyPairList;
  public:
    explicit RemoteResponder(cxxtools::http::Service& service)
      : cxxtools::http::Responder(service)
      {
        keyPairList = new KeyPairList(); 
      }
    ~RemoteResponder() { delete keyPairList; }
    
    virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<RemoteResponder> RemoteService;

