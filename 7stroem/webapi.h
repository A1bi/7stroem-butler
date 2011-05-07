// webapi.h
#ifndef _WEBAPI_H_
#define _WEBAPI_H_
#include <string>
#include "json.h"
using namespace std;

class WebAPI {
	public:
	void makeRequest(JSONobject*, string, JSONobject* = NULL);
	static void setServerAddr(string b) {
		serverAddr = b;
	}
	static void setAuthcode(string b) {
		authcode = b;
	}
	
	private:
	static string serverAddr, authcode;
	
};

#endif