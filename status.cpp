#include "status.h"
#include <unistd.h>
#include <stdio.h>

void StatusResponder::reply(std::ostream& out,
		cxxtools::http::Request& request, cxxtools::http::Reply& reply) {
	if (request.method() != "GET") {
		reply.httpReturn(403,
				"Only GET method is support by the remote control");
		return;
	}

	reply.addHeader("Content-Type", "text/plain; charset=utf-8");
	reply.addHeader("Transfer-Encoding", "chunked");

	chunked(out, "Volkers test für chuncked transfer - 1");
	sleep(10);
	chunked(out, "Volkers test für chuncked transfer - 2");
	sleep(10);
	chunked(out, "Volkers test für chuncked transfer - 2");
	sleep(10);
	chunked(out, "");
}

void StatusResponder::chunked(std::ostream& out, std::string chunk) {
	char *hex = NULL;
	if (asprintf(&hex, "%x\r\n", chunk.length()) > 0) {
		out << hex;
		out << chunk;
		out << "\r\n";
		out.flush();
		free (hex);
	}
}
