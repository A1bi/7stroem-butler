// game.h
#ifndef _GAME_H_
#define _GAME_H_
#include <string>
#include <vector>
#include <map>
using namespace std;
#include "player.h"
#include "card.h"

struct Action {
	string action;
	Player *player;
	string content;
	Action(string nAction, Player *nPlayer = NULL, string nContent = ""): action(nAction), player(nPlayer), content(nContent) {}
};

struct PlayerRequest {
	int playerId, since, sock;
	PlayerRequest(int p, int si, int so): playerId(p), since(si), sock(so) {}
};

class Game {
	public:
	// constructor: set gameId and create card deck
	Game() {
		// suits and numbers
		char suits[4] = { 'd', 's', 'h', 'c' };
		int numbers[8] = { 3, 4, 5, 6, 7, 8, 9, 10 };
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 8; j++) {
				// create card in card deck
				Card *newCard = new Card(suits[i], numbers[j]);
				allCards.push_back(newCard);
			}
		}
		possibleActions["layStack"] = 2;
		possibleActions["fold"] = 1;
		possibleActions["call"] = 4;
		possibleActions["knock"] = 3;
	}
	bool addPlayer(int playerId, string authcode);
	bool authenticate(int playerId, string authcode);
	pair<vector<Action*>, int> getActionsSince(int playerId, int start);
	bool registerAction(int PlayerId, string action, string content);
	void start();
	vector<PlayerRequest*> requestsWaiting; // TODO: private machen!!
	
	private:
	typedef vector<Player*> vPlayer;
	// contains id of game
	int gameId;
	// player list
	map<int, Player*> players;
	vPlayer activeRound, activeKnock;
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
	int smallRounds;
	Player* getPlayer(int playerId);
	void notifyAction(string action, Player *aPlayer, string content);
	void notifyAction(string action, Player *aPlayer);
	void giveCards();
	void nextTurn();
	void setTurn(Player *tPlayer);
	void startRound();
	void endRound();
	bool removePlayer(vPlayer &oPlayers, Player *delPlayer);

};

#endif