// game.h
#ifndef _GAME_H_
#define _GAME_H_
#include <string>
#include <vector>
using namespace std;
#include "card.h"
#include "gameapi.h"
#include "player.h"

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
	
	typedef vector<Player*> vPlayer;
	typedef vector<pPlayer> vpPlayer;
	
	public:
	// constructor: set id and create card deck
	Game(int, int);
	~Game();
	Player* addPlayer(int playerId, string authcode);
	bool removePlayer(vpPlayer::iterator);
	Player* authenticate(int playerId, string authcode);
	void getActionsSince(pair<vector<Action*>, int>*);
	bool registerAction(Player*, string action, string content);
	bool registerHostAction(Player*, string);
	int checkPlayers();
	int getId();
	int getHost() {
		return host;
	}
	
	private:
	// contains id of game
	const int gameId;
	int host;
	// player list
	vpPlayer players;
	vPlayer playersRound, playersSmallRound, activeKnock;
	int origPlayers;
	// whose turn is it, who did win last ?
	vPlayer::iterator turn, knockTurn;
	Player* lastWinner;
	// actions list
	vector<Action*> actions;
	// array containing the whole card deck
	vector<Card*> allCards;
	int turns, knocks, cardsLaid;
	bool started, roundStarted, finished, someonePoor, blindKnocked;
	GameAPI wAPI;
	
	void start();
	Player* getPlayer(int playerId);
	void notifyAction(string action, Player *aPlayer = NULL, string content = "");
	void notifyAction(string action, Player *aPlayer, int content);
	void notifyTurn(bool = false);
	void giveCards();
	void nextTurn();
	//void setTurn(Player *tPlayer);
	void startRound();
	void startSmallRound();
	void endSmallRound();
	void endRound();
	void knock(Player*, int);
	bool removePlayerFromList(vPlayer&, Player*, vPlayer::iterator* = NULL);
	void removeFromKnock(Player* = NULL);
	
};

#endif