#include "webapi.h"
using namespace std;
#include "httprequest.h"

// host of server
const string WebAPI::serverAddr = "7st.a0s.de";

void WebAPI::makeRequest(JSONobject* jsonRequest) {
	// prepare http request
	HTTPrequest request(serverAddr, "/butler.php");
	request.setPost("request", jsonRequest->str());
	
	// execute and get response
	HTTPresponse* response = request.execute();
	string rawResponse;
	response->getBody(&rawResponse);

}

void WebAPI::roundEnded(int game, int winner, int players) {
	JSONobject json;
	//json.addChild("game", game);
}