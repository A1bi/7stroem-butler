#include <sstream>
#include "card.h"
using namespace std;


// compare the suit of this card with the one of another card
// returns true if equal
bool Card::cmpSuitTo(Card *oCard) {
	if (suit == oCard->getSuit()) {
		return true;
	}
	return false;
}

// returns the suit
char Card::getSuit() {
	return suit;
}

// returns the card id of this card
// for example: heart 7 = h7
string Card::getCardId() {
	stringstream cardId;
	cardId << suit << number;
	return cardId.str();
}

// overload > operator so we can easily compare the numbers of two cards
bool Card::operator > (Card *c) {
	if (number > c->number) {
		return true;
	} else {
		return false;
	}	
}