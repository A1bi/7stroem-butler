// webapi.h
#ifndef _GAMEAPI_H_
#define _GAMEAPI_H_

#include <string>
#include <boost/function.hpp>
using namespace std;

#include "types.h"
#include "json.h"
#include "webapi.h"


class Game;
class Player;

class GameAPI : private WebAPI {
public:
	GameAPI(Game* const g): game(g) {}
	void roundEnded(PlayerPtr, int);
	void roundStarted();
	void playerQuit(PlayerPtr);
	void changeHost();
	void startGame();
	void finishGame();
	
private:
	Game* const game;
	void makeGameRequest(JSONobject*, string, boost::function<void()> = boost::function<void()>());
	
};

#endif