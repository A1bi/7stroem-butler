// httpresponse.h
#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_
#include <string>
#include <sstream>
#include "http.h"

using namespace std;


// class for response objects
class HTTPresponse : public HTTP {
public:
	HTTPresponse() : status(200) {};
	HTTPresponse(stringstream&);
	void setStatus(int);
	int getStatus();
	bool send(Socket*);
	void operator << (string);
	void getBody(string&);
	
private:
	// response status code
	unsigned int status;
	// contains body after the header
	string body;
	string generateHeader();
};

#endif