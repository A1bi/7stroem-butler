// webapi.h
#ifndef _GAMEAPI_H_
#define _GAMEAPI_H_
#include <string>
#include "json.h"
#include "webapi.h"
using namespace std;

class Game;
class Player;

class GameAPI : private WebAPI {
public:
	GameAPI(Game* const g): game(g) {}
	void roundEnded(Player*, int);
	void roundStarted();
	void playerQuit(Player*);
	void changeHost();
	void startGame();
	void finishGame();
	
private:
	Game* const game;
	void makeGameRequest(JSONobject*, string, JSONobject* = NULL);
	
};

#endif