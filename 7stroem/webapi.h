// webapi.h
#ifndef _WEBAPI_H_
#define _WEBAPI_H_
#include <string>
#include "json.h"
using namespace std;

class WebAPI {
	public:
	void roundEnded(int, int, int);
	void roundStarted(int);
	void playerQuit(int, int);
	
	private:
	void makeRequest(JSONobject*);
	static const string serverAddr;
	
};

#endif