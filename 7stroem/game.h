// game.h
#ifndef _GAME_H_
#define _GAME_H_
#include <string>
#include <vector>
#include <map>
#include "boost/thread/thread.hpp"
using namespace std;
#include "player.h"
#include "card.h"

class Game;

struct Action {
	string action;
	Player *player;
	string content;
	Action(string nAction, Player *nPlayer = NULL, string nContent = ""): action(nAction), player(nPlayer), content(nContent) {}
};

struct PlayerRequest {
	int since, sock;
	Game* game;
	Player* player;
	PlayerRequest(Game* g, Player* p, int si, int so): game(g), player(p), since(si), sock(so) {}
};

class ActionExcept {
public:
	ActionExcept(string msg, string id = ""): errorId(id), errorMsg(msg) {}
	string getErrorMsg() {
		return errorMsg;
	}
	string getErrorId() {
		return errorId;
	}
	
private:
	string errorId, errorMsg;
	
};

class Game {
	public:
	// constructor: set gameId and create card deck
	Game();
	bool addPlayer(int playerId, string authcode);
	Player* authenticate(int playerId, string authcode);
	pair<vector<Action*>, int> getActionsSince(Player*, int start);
	bool registerAction(Player*, string action, string content);
	void start();
	vector<PlayerRequest*> requestsWaiting; // TODO: private machen!!
	boost::mutex mutex;
	
	private:
	typedef vector<Player*> vPlayer;
	// contains id of game
	int gameId;
	// player list
	map<int, Player*> players;
	vPlayer playersRound, playersSmallRound, activeKnock;
	// whose turn is it, who did win last ?
	vPlayer::iterator turn;
	Player* lastWinner;
	// actions list
	vector<Action*> actions;
	// array containing the whole card deck
	vector<Card*> allCards;
	// possible actions (workaround for switch)
	map<string, int> possibleActions;
	// number of knocks players did in this round
	int knocks;
	int turns;
	bool roundStarted;
	Player* getPlayer(int playerId);
	void notifyAction(string action, Player *aPlayer = NULL, string content = "");
	void giveCards();
	void nextTurn();
	void setTurn(Player *tPlayer);
	void startRound();
	void startSmallRound();
	void endSmallRound();
	bool removePlayer(vPlayer &oPlayers, Player *delPlayer);

};

#endif