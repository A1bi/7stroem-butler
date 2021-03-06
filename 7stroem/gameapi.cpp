#include <sstream>
#include <boost/bind.hpp>

#include "gameapi.h"
#include "game.h"


void GameAPI::makeGameRequest(JSONobject* jsonRequest, string action, boost::function<void()> finishHandler) {
	// add game id to request
	stringstream value;
	value << game->getId();
	jsonRequest->addChild("game", value.str());
	
	makeRequest(jsonRequest, action, finishHandler);
}

void GameAPI::roundEnded(PlayerPtr winner, int players) {
	
	// prepare json
	JSONobject json;
	stringstream value;
	value << winner->getId();
	json.addChild("winner", value.str());
	value.str("");
	value << players;
	json.addChild("players", value.str());
	
	// send to server and receive response
	makeGameRequest(&json, "roundEnded");
}

void GameAPI::roundStarted() {
	// prepare json
	JSONobject json;
	// send to server and receive response
	//JSONobject responseJson;
	makeGameRequest(&json, "roundStarted");
	
	/*
	// check if any players have to be kicked because they have not enough money left
	int i = 0;
	string tmp;
	if (responseJson.getArray("kick") != NULL &&responseJson.getArray("kick")->getChild(i++, &tmp)) {
		//pla
	}
	*/
}

void GameAPI::playerQuit(PlayerPtr player) {
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << player->getId();
	json.addChild("player", value.str());
	
	// send to server and receive response
	makeGameRequest(&json, "playerQuit");
}

void GameAPI::changeHost() {
	// for type casting
	stringstream value;
	
	// prepare json
	JSONobject json;
	value << game->getHost();
	json.addChild("host", value.str());
	
	// send to server and receive response
	makeGameRequest(&json, "changeHost");
}

void GameAPI::finishGame() {	
	// prepare json
	JSONobject json;
	
	// send to server and receive response
	makeGameRequest(&json, "finishGame", boost::bind(&Game::kill, game));
}

void GameAPI::startGame() {
	// prepare json
	JSONobject json;
	
	// send to server and receive response
	makeGameRequest(&json, "startGame");
}