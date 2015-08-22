#include <cxxtools/http/request.h>
#include <cxxtools/http/reply.h>
#include <cxxtools/http/responder.h>
#include <locale.h>
#include <time.h>
#include "tools.h"

#ifndef WEBAPPESTFULAPI_H
#define WEBAPPRESTFULAPI_H

using namespace std;

class WebappResponder : public cxxtools::http::Responder {
private:
  string getFile(string base, string fileName);
  string getFileName(string base, string url);
  string getContentType(string fileName);
  string getBase(cxxtools::http::Request& request);
  void streamResponse(string fileName, ostream& out, string file, cxxtools::http::Reply& reply);
public:
  explicit WebappResponder(cxxtools::http::Service& service) : cxxtools::http::Responder(service) {};
  virtual void reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<WebappResponder> WebappService;

#endif // WEBAPPRESTFULAPI_H
