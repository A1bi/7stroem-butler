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
	HTTPrequest(stringstream&);
	HTTPrequest(const string, const string);
	string getAll();
	string getGet(string);
	void setGet(string, string);
	void setPost(string, string);
	//string getPost(string);
	string getUri() {
		return uri;
	}
	
private:
	// response status code
	string uri, host;
	string method;
	string generateHeader();
	map<string, string> get;
	void copyLines(HTTPresponse*, stringstream*);
	
};

#endif