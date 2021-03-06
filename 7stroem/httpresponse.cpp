#include <stdlib.h>
#include "http.h"
#include "httpresponse.h"

// generates response header
string HTTPresponse::generateHeader() {
	stringstream r;
	r << "HTTP/1.1 " << status << " OK\r\n";
	r << "Server: 7stroem-Butler/0.8 (Unix)\r\n";
	r << "Access-Control-Allow-Origin: *\r\n";
	r << "Pragma: no-cache\r\n";
	r << "Cache-Control: no-cache\r\n";
	r << "Expires: 0\r\n";
	r << "Content-Type: text/html\r\n";
	r << "Content-Length: " << body.size() << "\r\n";
	// header vars
	r << generateHeaderVars();
	r << "Connection: close\r\n\r\n";
	
	return r.str();
}

void HTTPresponse::setStatus(int newStatus) {
	status = newStatus;
}

int HTTPresponse::getStatus() {
	return status;
}

void HTTPresponse::operator << (string newContent) {
	body += newContent;
}

// parses an HTTP response
HTTPresponse::HTTPresponse(stringstream& data) {
	string line;
	
	// get status from first line
	getline(data, line);
	status = atoi(&line[9]);
	
	// parse the rest of the header
	parseHeader(data);
}

// just return the whole http response
string HTTPresponse::getAll() {
	return generateHeader() + body;
}

// copies the body into the given pointer
void HTTPresponse::getBody(string* const b) {
	*b = body;
}