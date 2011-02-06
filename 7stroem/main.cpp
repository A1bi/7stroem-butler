#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "game.h"
#include "socket.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "json.h"


int main (int argc, char * const argv[]) {
	
	vector<Action*>::iterator aIter;
	vector<Action*> *newActions;
	pair<vector<Action*>, int> actions;
	string gameRequest;
	
	// example
	Game *myGame = new Game;
	myGame->addPlayer(123, "bla");
	myGame->addPlayer(124, "bla");
	myGame->addPlayer(125, "bla");
	myGame->startGame();
	
	
	// initiate game server
	Socket gameServer;
	gameServer.bind(4926);
	gameServer.listen();
	
	// accepting connections from players
	Socket playerClient;
	gameServer.accept(playerClient);
	// getting request
	string rawRequest;
	playerClient.recv(rawRequest);
	HTTPrequest request(&rawRequest);
	
	// authenticate player!!!
	
	// responding to game request
	gameRequest = request.getGet("request");
	JSONobject response;
	
	
	// player wants to get actions
	if (gameRequest == "getActions") {
		
		// get since argument and check if not empty
		string since = request.getGet("since");
		if (since.empty()) return 0;
		// prepare response
		JSONarray* actionsArray = response.addArray("actions");
		
		// get all actions since since
		actions = myGame->getActionsSince(atoi(request.getGet("pId").c_str()), atoi(since.c_str()));
		// go through all actions
		newActions = &(actions.first);
		for (aIter = newActions->begin(); aIter != newActions->end(); ++aIter) {
			JSONobject* action = actionsArray->addObject();
			action->addChild("action", (*aIter)->action);
			stringstream pId;
			pId << (*aIter)->player->getId();
			action->addChild("player", pId.str());
			action->addChild("content", (*aIter)->content);
		}
		
		// conclude response with the number of the last action
		stringstream lastAction;
		lastAction << actions.second;
		response.addChild("lastAction", lastAction.str());
	}
	
	
	// initiate http response
	HTTPresponse rawResponse;
	
	// prepare json response and add to body
	rawResponse << "blubb = ";
	rawResponse << response.str();
	rawResponse << ";";
	// send response to player
	rawResponse.send(&playerClient);
	
	// close and cleanup everything
	playerClient.close();
	gameServer.close();
	exit(0);


	/*
	
	lastAction = showActions(123, lastAction);
	showActions(124, 0);
	showActions(125, 0);
	int player;
	while (action != "end") {
		cout << endl << "Spieler: ";
		cin >> player;
		cout << endl << "Aktion: ";
		cin >> action;
		cout << endl << "Inhalt: ";
		cin >> content;
		if (myGame->registerAction(player, action, content)) {
			cout << "yes!!" << endl;
			lastAction = showActions(123, lastAction);
		} else {
			cout << "nöö" << endl;
		}
	}
	*/
	
    return 0;
}