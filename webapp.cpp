#include "webapp.h"

void WebappResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler::addHeader(reply);

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET");
      reply.httpReturn(200, "OK");
      return;
  }
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve files use the GET method!");
     return;
  }

  double timediff = -1;
  string url = request.url();
  string base = getBase(request);

  getBase(request);

  if ( base == request.url() ) {
      reply.addHeader("Location", (base + "/").c_str());
      reply.httpReturn(301, "Moved Permanently");
      return;
  }

  if ( (int)url.find(base) == 0) {

      esyslog("restfulapi Webapp: file request url %s", request.url().c_str());

      string fileName = getFileName(base, url);
      string file = getFile(base, fileName);

      if (!FileExtension::get()->exists(file)) {
	  esyslog("restfulapi Webapp: file does not exist");
	  reply.httpReturn(404, "File not found");
	  return;
      }

      if (request.hasHeader("If-Modified-Since")) {
	  timediff = difftime(FileExtension::get()->getModifiedTime(file), FileExtension::get()->getModifiedSinceTime(request));
      }
      if (timediff > 0.0 || timediff < 0.0) {
	  streamResponse(fileName, out, file, reply);
      } else {
	  esyslog("restfulapi Webapp: file not modified, returning 304");
	  reply.httpReturn(304, "Not-Modified");
      }
  }
}

/**
 * retrieve base of webapp
 */
string WebappResponder::getBase(cxxtools::http::Request& request) {

  string url = request.url();

  if (url.find_first_of("/") == 0) {
      url = url.substr(1, url.length());
  }

  return "/" + url.substr(0, url.find_first_of("/"));
};

/**
 * retrieve filename width path
 * @param string fileName
 */
string WebappResponder::getFile(string base, std::string fileName) {


  map<string, string> webapps = Settings::get()->Webapps();
  map<string, string>::iterator it;
  string webappPath;

  it = webapps.find(base.substr(1));
  webappPath = it->second;

  if ( webappPath.find_last_of("/") == (webappPath.length() - 1) ) {
      webappPath = webappPath.substr(0, webappPath.length() - 1);
  }

  if ( fileName.find_first_of("/") == 0 ) {
      fileName = fileName.substr(1, fileName.length() - 1);
  }

  return webappPath + (string)"/" + fileName;
};

/**
 * retrieve filename
 * @param string base url
 * @param string url
 */
string WebappResponder::getFileName(string base, string url) {

  if ( url.find_last_of("/") == (url.length() - 1) ) {
      url = url.substr(0, url.length() - 1);
  }

  string file = url.replace(0, base.length(), "");

  if ( file != "" && file.find(".") == string::npos ) {

      file = file + "/index.html";

  } else if ( file == "" ) {

      file = "index.html";
  }

  esyslog("restfulapi: requested file %s", file.c_str());

  return file;
};

/**
 * determine contenttype
 * @param string fileName
 */
string WebappResponder::getContentType(string fileName) {

  map<string, string> types = Settings::get()->WebappFileTypes();
  map<string, string>::iterator it;

  string type = fileName.substr(fileName.find_last_of(".")+1);
  string contentType = "application/octet-stream";
  esyslog("restfulapi Webapp: file extension of %s is '%s'", fileName.c_str(), type.c_str());

  it = types.find(type);
  if (it != types.end()) {
      contentType = it->second;
  }

  esyslog("restfulapi Webapp: file type of %s is '%s'", fileName.c_str(), contentType.c_str());

  return contentType;
};

/**
 * stream file
 */
void WebappResponder::streamResponse(string fileName, ostream& out, string file, cxxtools::http::Reply& reply) {

  string contentType = getContentType(fileName);

  StreamExtension se(&out);
  if ( contentType  != "" && se.writeBinary(file) ) {
      esyslog("restfulapi Webapp: successfully piped file %s", fileName.c_str());
      FileExtension::get()->addModifiedHeader(file, reply);
      reply.addHeader("Content-Type", contentType.c_str());
  } else {
      esyslog("restfulapi Webapp: error piping file %s", fileName.c_str());
      reply.httpReturn(404, "File not found");
  }
};
