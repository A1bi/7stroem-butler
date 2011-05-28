#include <iostream>
#include <boost/bind.hpp>
using namespace std;

#include "player.h"
#include "game.h"


// constructor
Player::Player(int i, string a, Game* g, boost::asio::io_service* io): PlayerId(i), authcode(a), game(g), timerDisconnect(*io) {
	connected = 1;
	quitted = false;
	setDisconnected();
}

// returns player id
int Player::getId() {
	return PlayerId;
}

// return game object this player belongs to
Game* Player::getGame() {
	return game;
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
	if (folded || !flipped) return false;
	
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

// reset strikes before a new round
void Player::newRound() {
	strikes = 0;
}

// reset a few things when starting new round
void Player::newSmallRound() {
	// erase all cards from (actually there shouldn't be any left)
	hand.clear();
	// erase all cards from stack
	stack.clear();
	// other things
	folded = false;
	flipped = false;
	knockPossible = true;
	calls = 1;
}

// player knocks
bool Player::knock(const int knocks) {
	if (!knockPossible) {
		throw ActionExcept("you have to call another player's knock before you can knock again");
	} else if (strikes > 5) {
		throw ActionExcept("you cannot knock when you are poor");
	} else if (strikes+calls+knocks > ((knocks > 1) ? 8 : 7)) {
		throw ActionExcept("you have too many strikes for this action");
	}
	// also increase calls because he obviously has to call his own knock
	knockPossible = false;
	calls += knocks;
	return true;
}

// player knocks blindly
bool Player::blindKnock(const int knocks) {
	// check if not already flipped
	if (!flipped) {
		knock(knocks-1);
		return true;
	}
	throw ActionExcept("you have already seen your cards");
}

// player calls afters another player knocked
void Player::call(const int c) {
	calls += c;
	knockPossible = true;
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
}

// TODO: get rid of win()
// player wins
void Player::win() {
	newSmallRound();
}

// return number of strikes
int Player::getStrikes() {
	return strikes;
}

// mark player as connected
void Player::setConnected() {
	// cancel timer
	timerDisconnect.cancel();
	// increase count of clients that are connected
	connected++;
}

// mark player as disconnected and note the time of his disconnection
void Player::setDisconnected() {
	// decrease count of clients that are connected
	if (--connected < 1) {
		// player is not connected anymore
		// he has now 15 seconds to reconnect or this timer will remove him from the game
		timerDisconnect.expires_from_now(boost::posix_time::seconds(15));
		timerDisconnect.async_wait(boost::bind(&Player::handleDisconnect, this, boost::asio::placeholders::error));
	}
}

void Player::handleDisconnect(const boost::system::error_code& error) {
	if (error) return;
	
	std::cout << "player disconnected (timeout)" << std::endl;
	quit();
}

void Player::quit() {
	std::cout << "player disconnected (wanted)" << std::endl;
	quitted = true;
	game->removePlayer(PlayerId);
}

// get cards in player's hand as string separated by a comma
void Player::getHand(string cards[4]) {
	int i = 0;
	for (sCard::iterator cIter = hand.begin(); cIter != hand.end(); ++cIter) {
		cards[i] = (*cIter)->getCardId();
		i++;
	}
}

// note that player has already seen his cards
void Player::flipHand() {
	flipped = true;
}

// just returns the number of cards the player has laid on stack
int Player::cardsOnStack() {
	return stack.size();
}