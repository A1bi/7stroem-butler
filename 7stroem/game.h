// game.h
#ifndef _GAME_H_
#define _GAME_H_

#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
using namespace std;

#include "types.h"
#include "card.h"
#include "gameapi.h"
#include "player.h"

class Server;

struct Action {
	const string action;
	const int player;
	const string content;
	Action(string nAction, PlayerPtr aPlayer, string nContent): action(nAction), player((aPlayer == PlayerPtr()) ? 0 : aPlayer->getId()), content(nContent) {}
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
	PlayerPtr second;
	pPlayer(int i, PlayerPtr p) : first(i), second(p) {}
	bool operator == (int i) {
		return (i == first);
	}
};

class Game : public boost::enable_shared_from_this<Game> {
	
	typedef vector<PlayerPtr> vPlayer;
	typedef vector<pPlayer> vpPlayer;
	
	public:
	connectionSet requestsWaiting;
	
	// constructor: set id and create card deck
	Game(int, int, Server*);
	~Game();
	bool addPlayer(int playerId, string authcode);
	void removePlayer(int);
	PlayerPtr authenticate(int playerId, string authcode);
	void getActionsSince(pair<vector<Action*>, int>*);
	bool registerAction(PlayerPtr, string action, string content);
	bool registerHostAction(PlayerPtr, string);
	int getId();
	int getHost() {
		return host;
	}
	void kill();
	
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
	PlayerPtr lastWinner;
	// actions list
	vector<Action*> actions;
	// array containing the whole card deck
	vector<Card*> allCards;
	int rounds, turns, knocks, cardsLaid;
	bool started, roundStarted, finished, someonePoor, blindKnocked;
	GameAPI wAPI;
	Server* server;
	
	void start();
	PlayerPtr getPlayer(int playerId);
	void notifyAction(string action, PlayerPtr aPlayer = PlayerPtr(), string content = "");
	void notifyAction(string action, PlayerPtr aPlayer, int content);
	void notifyTurn(bool = false);
	void giveCards();
	void nextTurn();
	//void setTurn(Player *tPlayer);
	void startRound();
	void startSmallRound();
	void endSmallRound();
	void endRound();
	void knock(PlayerPtr, int);
	bool removePlayerFromList(vPlayer&, PlayerPtr, vPlayer::iterator* = NULL);
	void removeFromKnock(PlayerPtr = PlayerPtr());
	void killQuitPlayers(vpPlayer::iterator* = NULL);
	
};

#endif