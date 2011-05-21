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
	HTTPresponse() : status(200) {
		setHeaderValue("Connection", "close");
	}
	HTTPresponse(stringstream&);
	void setStatus(int);
	int getStatus();
	string getAll();
	void operator << (string);
	void getBody(string* const);
	
private:
	// response status code
	unsigned int status;
	string generateHeader();
};

#endif