// player.h
#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <string>
#include <set>
#include <boost/asio.hpp>
using namespace std;

#include "card.h"


class Game;

class Player {
	public:
	// constructor: set id of user and the code required to authenticate a user
	Player(int, string, Game*, boost::asio::io_service*);
	~Player() {
		timerDisconnect.cancel();
	}
	bool authenticate(int pid, string authcode);
	void giveCard(Card *givenCard);
	bool layStack(Card *hCard);
	bool knock(const int = 1);
	bool blindKnock(const int);
	bool fold();
	void call(const int);
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
	void newRound();
	void newSmallRound();
	void getHand(string[4]);
	void flipHand();
	int cardsOnStack();
	void quit();
	bool hasFolded() {
		return folded;
	}
	bool hasQuit() {
		return quitted;
	}
	
	private:
	const int PlayerId;
	const string authcode;
	Game* const game;
	typedef set<Card*> sCard;
	sCard hand;
	vector<Card*> stack;
	// use quitted instead of quit because otherwise it would conflict with the function quit()
	bool folded, quitted;
	bool flipped;
	bool knockPossible;
	int strikes;
	int calls;
	int connected;
	boost::asio::deadline_timer timerDisconnect;
	
	void handleDisconnect(const boost::system::error_code&);
	void incStrikes(int newStrikes);

};

#endif