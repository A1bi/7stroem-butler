#include "webapi.h"
#include "httprequest.h"
using namespace std;

// host of server
string WebAPI::serverAddr = "7st.a0s.de";
string WebAPI::authcode = "";

void WebAPI::makeRequest(JSONobject* jsonRequest, string action, JSONobject* responseJson) {
	// add authcode and request
	jsonRequest->addChild("request", action);
	jsonRequest->addChild("authcode", authcode);
	
	// prepare http request
	HTTPrequest request(serverAddr, "/butler.php");
	request.setPost("request", jsonRequest->str());
	
	// execute and get response
	HTTPresponse* response = request.execute();
	string rawResponse;
	if (response != NULL) {
		response->getBody(&rawResponse);
		delete response;
	}
	
	// parse json
	if (responseJson != NULL) {
		responseJson->parse(rawResponse);
	}
}