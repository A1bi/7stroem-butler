// http.h
#ifndef _HTTP_H_
#define _HTTP_H_
#include <string>
#include <map>
#include <vector>
#include "socket.h"
using namespace std;


// main http parsing class
class HTTP {
public:
	string getHeaderValue(string);
	void setHeaderValue(string, string);
	
protected:
	// contains all header vars with their values
	map<string, string> headerVars;
	// parses the header
	void parseHeader(stringstream&);
	vector<string> rawCookies;
	string generateHeaderVars();
	string urlEncode(const string&);
	// contains body after the header
	string body;
	
};

#endif