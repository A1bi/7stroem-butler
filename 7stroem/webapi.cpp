#include "webapi.h"
using namespace std;
#include "httprequest.h"

// host of server
const string WebAPI::serverAddr = "7st.a0s.de";
const string WebAPI::authcode = "kqecbi50PLZvDkyp";

JSONobject WebAPI::makeRequest(JSONobject* jsonRequest, string action) {
	// add request and authcode to json object
	jsonRequest->addChild("authcode", authcode);
	jsonRequest->addChild("request", action);
	
	// prepare http request
	HTTPrequest request(serverAddr, "/butler.php");
	request.setPost("request", jsonRequest->str());
	
	// execute and get response
	HTTPresponse* response = request.execute();
	string rawResponse;
	response->getBody(&rawResponse);
	// parse json
	JSONobject responseJson(rawResponse);
	
	delete response;
	return responseJson;
}

void WebAPI::roundEnded(int game, int winner, int players) {
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << game;
	json.addChild("game", value.str());
	value.str("");
	value << winner;
	json.addChild("winner", value.str());
	value.str("");
	value << players;
	json.addChild("players", value.str());
	
	// send to server and receive response
	makeRequest(&json, "roundEnded");
}

void WebAPI::roundStarted(int game) {	
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << game;
	json.addChild("game", value.str());
	
	// send to server and receive response
	JSONobject responseJson = makeRequest(&json, "roundStarted");
	
	// check if any players have to be kicked because they have not enough money left
	int i;
	string tmp;
	if (responseJson.getArray("kick")->getChild(i++, &tmp)) {
		//pla
	}
}

void WebAPI::playerQuit(int game, int player) {
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << game;
	json.addChild("game", value.str());
	value.str("");
	value << player;
	json.addChild("player", value.str());
	
	// send to server and receive response
	makeRequest(&json, "playerQuit");
}