#include "webapi.h"
#include "game.h"
#include "httprequest.h"
using namespace std;

// host of server
const string WebAPI::serverAddr = "7st.a0s.de";
const string WebAPI::authcode = "kqecbi50PLZvDkyp";

JSONobject WebAPI::makeRequest(JSONobject* jsonRequest, string action) {
	// add request and authcode to json object
	jsonRequest->addChild("authcode", authcode);
	jsonRequest->addChild("request", action);
	stringstream value;
	value << game->getId();
	jsonRequest->addChild("game", value.str());
	
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
	JSONobject responseJson(rawResponse);		
	
	return responseJson;
}

void WebAPI::roundEnded(Player* winner, int players) {

	// prepare json
	JSONobject json;
	stringstream value;
	value << winner->getId();
	json.addChild("winner", value.str());
	value.str("");
	value << players;
	json.addChild("players", value.str());
	
	// send to server and receive response
	makeRequest(&json, "roundEnded");
}

void WebAPI::roundStarted() {
	// prepare json
	JSONobject json;
	
	// send to server and receive response
	JSONobject responseJson = makeRequest(&json, "roundStarted");
	
	// check if any players have to be kicked because they have not enough money left
	int i = 0;
	string tmp;
	if (responseJson.getArray("kick") != NULL &&responseJson.getArray("kick")->getChild(i++, &tmp)) {
		//pla
	}
}

void WebAPI::playerQuit(Player* player) {
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << player->getId();
	json.addChild("player", value.str());
	
	// send to server and receive response
	makeRequest(&json, "playerQuit");
}

void WebAPI::changeHost() {
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << game->getHost();
	json.addChild("host", value.str());
	
	// send to server and receive response
	makeRequest(&json, "changeHost");
}

void WebAPI::finishGame() {	
	// prepare json
	JSONobject json;
	
	// send to server and receive response
	makeRequest(&json, "finishGame");
}

void WebAPI::startGame() {
	// prepare json
	JSONobject json;
	
	// send to server and receive response
	makeRequest(&json, "startGame");
}