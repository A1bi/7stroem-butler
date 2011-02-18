// player.h
#ifndef _PLAYER_H_
#define _PLAYER_H_
#include <string>
#include <set>
using namespace std;
#include "card.h"

class Player {
	public:
	// constructor: set id of user and the code required to authenticate a user
	Player(int pId, string pAuthcode): PlayerId(pId), authcode(pAuthcode) {
		strikes = 0;
		newRound();
	}
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
	
	private:
	const int PlayerId;
	typedef set<Card*> sCard;
	const string authcode;
	sCard hand;
	vector<Card*> stack;
	bool folded;
	int strikes;
	int calls;
	bool knocked;
	void newRound();
	void incStrikes(int newStrikes);

};

#endif