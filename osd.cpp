#include "osd.h"

void OsdResponder::reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  if ( request.method() != "GET" ) {
     reply.httpReturn(403, "Only GET-method is supported!");
     return;
  }
  StreamExtension se(&out);
  se.write("To be implemented...");
}

