// webapi.h
#ifndef _WEBAPI_H_
#define _WEBAPI_H_
#include <string>
#include "json.h"
using namespace std;

class Game;
class Player;

class WebAPI {
	public:
	WebAPI(Game* const g): game(g) {}
	void roundEnded(Player*, int);
	void roundStarted();
	void playerQuit(Player*);
	void changeHost();
	void startGame();
	void finishGame();
	
	private:
	Game* const game;
	JSONobject makeRequest(JSONobject*, string);
	static const string serverAddr, authcode;
	
};

#endif