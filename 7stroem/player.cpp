#include <vector>
#include <set>
#include <string>
using namespace std;
#include "player.h"

// constructor
Player::Player(int pId, string pAuthcode): PlayerId(pId), authcode(pAuthcode) {
	strikes = 0;
	newRound();
}

// returns player id
int Player::getId() {
	return PlayerId;
}

// check if given authcode of the player is correct
bool Player::authenticate(int pid, string authcode) {
	return (pid == PlayerId && Player::authcode == authcode);
}

// user has got cards which now are his hand
void Player::giveCard(Card *givenCard) {
	hand.insert(givenCard);
}

// player folds ?
bool Player::fold() {
	// not already folded
	if (!folded) {
		folded = true;
		// player has lost
		lose();
		return true;
	}
	return false;
}

// player lays a card on stack
bool Player::layStack(Card *hCard) {
	// already folded ?
	if (folded) return false;
	
	// take card from hand
	int del = hand.erase(hCard);
	// card was not in hand ?
	if (!del) return false;
	
	// lay card on stack
	stack.push_back(hCard);
	
	return true;
}

// search for a cardId in player's hand and return refernce to the correct Card object
Card* Player::cardFromHand(string cardId) {
	sCard::iterator hIter;
	for (hIter = hand.begin(); hIter != hand.end(); ++hIter) {
		if ((*hIter)->getCardId() == cardId) {
			return *hIter;
		}
	}
	return NULL;
}

// look for a specific suit in hand
bool Player::checkForSuit(Card *givenCard) {
	sCard::iterator cIter;
	for (cIter = hand.begin(); cIter != hand.end(); ++cIter) {
		// compare suits
		if ((*cIter)->cmpSuitTo(givenCard)) {
			return true;
		}
	}
	return false;
}

//reset a few things when starting new round
void Player::newRound() {
	// erase all cards from (actually there shouldn't be any left)
	hand.clear();
	// erase all cards from stack
	stack.clear();
	// other things
	folded = false;
	knocked = false;
	calls = 1;
}

// player knocks
bool Player::knock() {
	// player can only knock once and if he has less than 6 strikes
	if (knocked || strikes > 6) {
		return false;
	}
	// also increase calls because he obviously has to call his own knock
	call();
	return true;
}

// player calls afters another player knocked
void Player::call() {
	calls++;
}

// return last card on top of player's stack
Card* Player::lastStack() {
	return stack.back();
}

// after player lost a round he gets a number of strikes added
void Player::incStrikes(int newStrikes) {
	strikes += newStrikes;
}

// player loses
void Player::lose() {
	incStrikes(calls);
	newRound();
}

// player wins
void Player::win() {
	newRound();
}

// return number of strikes
int Player::getStrikes() {
	return strikes;
}