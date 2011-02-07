// httprequest.h
#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_
#include <string>
#include <map>
#include "http.h"
#include "httpresponse.h"
using namespace std;


// class for request objects
class HTTPrequest : public HTTP {
public:
	HTTPrequest(string*);
	HTTPrequest(string, string);
	HTTPresponse* execute();
	string getGet(string);
	void setGet(string, string);
	string getUri() {
		return uri;
	}
	
private:
	// response status code
	string method, uri, host;
	string generateHeader();
	map<string, string> get;
	void copyLines(HTTPresponse*, stringstream&);
	void copyLines(HTTPresponse*, stringstream&, int&);
};

#endif