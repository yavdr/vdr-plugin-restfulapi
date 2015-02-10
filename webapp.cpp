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
  string base = "/webapp";

  if ( base == request.url() ) {
      reply.addHeader("Location", "/webapp/");
      reply.httpReturn(301, "Moved Permanently");
      return;
  }

  if ( (int)url.find(base) == 0) {

      esyslog("restfulapi Webapp: file request url %s", request.url().c_str());

      string fileName = getFileName(base, url);
      string file = getFile(fileName);

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
 * retrieve filename width path
 * @param string fileName
 */
string WebappResponder::getFile(std::string fileName) {

  string webappPath = Settings::get()->WebappDirectory();

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

  const char *empty = "";

  if ( url.find_last_of("/") == (url.length() - 1) ) {
      url = url.substr(0, url.length() - 1);
  }
  string file = url.replace(0, base.length(), "");

  if (strcmp(file.c_str(), empty) == 0) {
      file = "index.html";
  }
  return file;
};

/**
 * determine contenttype
 * @param string fileName
 */
const char *WebappResponder::getContentType(string fileName) {

  const char *type = fileName.substr(fileName.find_last_of(".")+1).c_str();
  const char *contentType = "";
  const char *extHtml = "html";
  const char *extJs = "js";
  const char *extCss = "css";
  const char *extJpg = "jpg";
  const char *extJpeg = "jpeg";
  const char *extGif = "gif";
  const char *extPng = "png";
  const char *extIco = "ico";
  const char *extAppCacheManifest = "appcache";
  esyslog("restfulapi Webapp: file extension of %s is %s", fileName.c_str(), type);

  if ( strcmp(type, extHtml) == 0 ) {
      contentType = "text/html";
  } else if ( strcmp(type, extJs) == 0 ) {
      contentType = "application/javascript";
  } else if ( strcmp(type, extCss) == 0 ) {
      contentType = "text/css";
  } else if ( strcmp(type, extJpg) == 0 || strcmp(type, extJpeg) == 0 || strcmp(type, extGif) == 0 || strcmp(type, extPng) == 0 ) {
      contentType = ("image/" + (string)type).c_str();
  } else if ( strcmp(type, extIco) == 0 ) {
      contentType = "image/x-icon";
  } else if ( strcmp(type, extAppCacheManifest) == 0 ) {
      contentType = "text/cache-manifest";
  }
  esyslog("restfulapi Webapp: file type of %s is %s", fileName.c_str(), contentType);

  return contentType;
};

void WebappResponder::streamResponse(string fileName, ostream& out, string file, cxxtools::http::Reply& reply) {

  const char *empty = "";
  const char * contentType = getContentType(fileName);

  StreamExtension se(&out);
  if ( strcmp(contentType, empty) != 0 && se.writeBinary(file) ) {
      esyslog("restfulapi Webapp: successfully piped file %s", fileName.c_str());
      FileExtension::get()->addModifiedHeader(file, reply);
      reply.addHeader("Content-Type", contentType);
  } else {
      esyslog("restfulapi Webapp: error piping file %s", fileName.c_str());
      reply.httpReturn(404, "File not found");
  }
};
