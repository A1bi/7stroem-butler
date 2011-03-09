// card.h
#ifndef _CARD_H_
#define _CARD_H_
#include <string>
using namespace std;

// object for one card
class Card {
	public:
	// constructor
	Card(char nSuit, int nNumber): suit(nSuit), number(nNumber) {}
	bool cmpSuitTo(Card *oCard);
	char getSuit();
	bool operator > (Card &c);
	string getCardId();
	
	private:
	const char suit;
	const int number;

};

#endif