// game.h
#ifndef _GAME_H_
#define _GAME_H_
#include <string>
#include <vector>
using namespace std;
#include "card.h"
#include "player.h"
#include "webapi.h"


struct Action {
	const string action;
	const int player;
	const string content;
	Action(string nAction, Player* aPlayer, string nContent): action(nAction), player((aPlayer == NULL) ? 0 : aPlayer->getId()), content(nContent) {}
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
	const string errorId, errorMsg;
	
};

struct pPlayer {
	int first;
	Player* second;
	pPlayer(int i, Player* p) : first(i), second(p) {}
	bool operator == (int i) {
		return (i == first);
	}
};

class Game {
	public:
	// constructor: set id and create card deck
	Game(int, int);
	~Game();
	Player* addPlayer(int playerId, string authcode);
	bool removePlayer(Player* player);
	Player* authenticate(int playerId, string authcode);
	void getActionsSince(pair<vector<Action*>, int>*);
	bool registerAction(Player*, string action, string content);
	bool registerHostAction(Player*, string);
	int getId();
	int getHost() {
		return host;
	}
	
	private:
	typedef vector<Player*> vPlayer;
	typedef vector<pPlayer> vpPlayer;
	// contains id of game
	const int gameId;
	int host;
	// player list
	vpPlayer players;
	vPlayer playersRound, playersSmallRound, activeKnock;
	int origPlayers;
	// whose turn is it, who did win last ?
	vPlayer::iterator turn;
	Player* lastWinner;
	// actions list
	vector<Action*> actions;
	// array containing the whole card deck
	vector<Card*> allCards;
	// number of knocks players did in this round
	// TODO: get rid of knocks
	int knocks;
	int turns;
	bool started, roundStarted, someonePoor, blindKnocked;
	WebAPI wAPI;
	
	void start();
	Player* getPlayer(int playerId);
	void notifyAction(string action, Player *aPlayer = NULL, string content = "");
	void notifyAction(string action, Player *aPlayer, int content);
	void giveCards();
	void nextTurn();
	void setTurn(Player *tPlayer);
	void startRound();
	void startSmallRound();
	void endSmallRound();
	void endRound();
	void knock(Player*, int);
	bool removePlayerFromList(vPlayer &oPlayers, Player *delPlayer);
	void removeFromKnock(Player*);
	
};

#endif