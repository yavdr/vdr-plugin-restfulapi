#include "webapp.h"
using namespace std;


void WebappResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  double timediff = -1;
  string base = "/webapp/";
  string webappPath = Settings::get()->WebappDirectory();
  string url = request.url();
  const char *extHtml = "html";
  const char *extJs = "js";
  const char *extCss = "css";
  const char *type;

  if ( (int)url.find(base) == 0 ) {

      esyslog("restfulapi Webapp: file request url %s", request.url().c_str());

      QueryHandler::addHeader(reply);
      string file = url.replace(0, base.length(), "");

      const char *fileCmp = file.c_str();
      const char *empty = "";

      if (strcmp(fileCmp, empty) == 0) {
	  file = "index.html";
      }

      string path = webappPath + (string)"/" + file;

      if (!FileExtension::get()->exists(path)) {
	  esyslog("restfulapi Webapp: file does not exist");
	  reply.httpReturn(404, "File not found");
	  return;
      }

      if (request.hasHeader("If-Modified-Since")) {
	  timediff = difftime(FileExtension::get()->getModifiedTime(path), FileExtension::get()->getModifiedSinceTime(request));
      }
      if (timediff > 0.0 || timediff < 0.0) {
	  type = file.substr(file.find_last_of(".")+1).c_str();
	  esyslog("restfulapi Webapp: file type %s", type);
	  string contenttype;

	  if ( strcmp(type, extHtml) == 0 ) {

	      contenttype = (string)"text/html";

	  } else if ( strcmp(type, extJs) == 0 ) {

	      contenttype = (string)"application/javascript";

	  } else if ( strcmp(type, extCss) == 0 ) {

	      contenttype = (string)"text/css";

	  } else {
	      contenttype = (string)"image/" + type;
	  }

	  StreamExtension se(&out);
	  if ( se.writeBinary(path) ) {
	      esyslog("restfulapi Webapp: successfully piped file");
	      FileExtension::get()->addModifiedHeader(path, reply);
	      reply.addHeader("Content-Type", contenttype.c_str());
	  } else {
	      esyslog("restfulapi Webapp: error piping file");
	      reply.httpReturn(404, "File not found");
	  }
      } else {
	  esyslog("restfulapi Webapp: file not modified, returning 304");
	  reply.httpReturn(304, "Not-Modified");
      }
  }
}
