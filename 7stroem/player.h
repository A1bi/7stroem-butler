// player.h
#ifndef _PLAYER_H_
#define _PLAYER_H_
#include <string>
#include <set>
using namespace std;
#include "card.h"
#include "game.h"

class Game;

class Player {
	public:
	// constructor: set id of user and the code required to authenticate a user
	Player(int, string, Game*);
	bool authenticate(int pid, string authcode);
	void giveCard(Card *givenCard);
	bool layStack(Card *hCard);
	bool knock();
	bool fold();
	void call();
	void win();
	void lose();
	Card* lastStack();
	Card* cardFromHand(string cardId);
	bool checkForSuit(Card* givenCard);
	int getId();
	Game* getGame();
	int getStrikes();
	void setConnected();
	void setDisconnected();
	bool isConnected();
	bool isMissing();
	void newRound();
	void newSmallRound();
	void getHand(string[4]);
	void flipHand();
	
	private:
	const int PlayerId;
	const string authcode;
	Game* const game;
	typedef set<Card*> sCard;
	sCard hand;
	vector<Card*> stack;
	bool folded;
	bool flipped;
	int strikes;
	int calls;
	bool knocked;
	int connected;
	int lastSeen;
	
	void incStrikes(int newStrikes);

};

#endif