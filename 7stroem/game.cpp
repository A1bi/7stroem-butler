#include <string>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
#include "game.h"

// TODO: Exceptions und Fehlerausgabe als JSON!!
// TODO: notifyAction() für content als int überladen

// constructor
Game::Game() {
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

// start the game
void Game::start() {
	// initial turn
	turn = activeRound.begin();
	// notify game has started
	notifyAction("started", *turn);
	// start first round
	startRound();
}

// start a round
void Game::startRound() {
	// notify that round has started
	notifyAction("roundStarted", *turn);
	startSmallRound();
}

// start a round
void Game::startSmallRound() {
	turns = 1;
	// new winner is now the player who is first
	lastWinner = *turn;
	// notify that small round has started
	notifyAction("smallRoundStarted", lastWinner);
	giveCards();
	// notify first turn
	notifyAction("turn", *turn);
}

// end a round
void Game::endSmallRound() {
	lastWinner->win();
	notifyAction("smallRoundEnded", lastWinner);
	// before we reset we have to save the current turn's pointer so we can find the player afterwards
	Player* oldTurn = *turn;
	// reset to all players active
	activeRound.clear();
	map<int, Player*>::iterator pIter;
	stringstream strikes;
	for (pIter = players.begin(); pIter != players.end(); ++pIter) {
		activeRound.push_back(pIter->second);
		strikes << pIter->second->getStrikes();
		notifyAction("strikes", pIter->second, strikes.str());
		strikes.str("");
		strikes.clear();
	}
	// since we recreated activeRound we now have to find back the turn we had before using oldTurn
	// also increase to get next turn
	turn = find(activeRound.begin(), activeRound.end(), oldTurn) + 1;
	if (turn == activeRound.end()) {
		turn = activeRound.begin();
	}
	startSmallRound();
}

// adds player to players list
bool Game::addPlayer(int playerId, string authcode) {
	// check if player not yet added
	if (players.find(playerId) != players.end()) {
		return false;
	}
	// create pointer to new Player object
	Player *newPlayer = new Player(playerId, authcode);
	// insert into vector
	players[playerId] = newPlayer;
	activeRound.push_back(newPlayer);
	// register as action
	notifyAction("playerJoined", newPlayer);
	
	return true;
}

// register an action
bool Game::registerAction(int PlayerId, string action, string content) {
	// get Player object
	Player *tPlayer = getPlayer(PlayerId);
	if (!tPlayer) {
		// player not found
		return false;
	}
	
	// chat
	if (action == "chat") {
		notifyAction("chat", tPlayer, content);
		return true;
	}
	
	// TODO: FIX: bad access vor spielbeginn
	// is it the player's turn and he doesn't want to fold ?
	if ( ((tPlayer != *turn || activeKnock.empty() && action == "call") && action != "fold" && action != "call") || find(activeRound.begin(), activeRound.end(), tPlayer) == activeRound.end() ) {
		// not allowed here -> false
		return false;
	}
	
	switch (possibleActions[action]) {
			
		// fold
		case 1: {
			// fold cards -> quit round
			if (tPlayer->fold()) {
				notifyAction("folded", tPlayer);
				// remove player if there is an active knock
				if (!activeKnock.empty()) {
					removePlayer(activeKnock, tPlayer);
				}
				
				// save current turn for later
				Player* oldTurn = *turn;
				// remove player from active list
				removePlayer(activeRound, tPlayer);
				if (activeRound.size() == 1) {
					// only one player left -> end round
					lastWinner = activeRound.front();
					setTurn(lastWinner);
					endSmallRound();
					
				} else {
					setTurn(oldTurn);
					// it was this player's turn so we have to go to the next player
					if (oldTurn == tPlayer) {
						nextTurn();
						notifyAction("turn", *turn);
					}
					// last winner is now the player next to this player
					if (lastWinner == tPlayer) {
						if (turn == activeRound.end()) {
							lastWinner = activeRound.front();
						} else {
							lastWinner = *turn;
						}
					}
				}
				
				return true;
			}
			break;
		}
			
		// layStack
		case 2: {
			// get Card object
			Card* lCard = tPlayer->cardFromHand(content);
			if (!lCard) {
				return false;
			}
			
			// check if player laid a suit different to the leading although he has the correct suit in hand -> not permitted
			if (tPlayer != lastWinner && !lastWinner->lastStack()->cmpSuitTo(lCard) && tPlayer->checkForSuit(lastWinner->lastStack())) {
				return false;
			}
			
			// try to lay the card on stack
			if (tPlayer->layStack(lCard)) {
				notifyAction("laidStack", tPlayer, lCard->getCardId());
				nextTurn();
				// small round complete
				if (*turn == lastWinner) {
					// check who won
					vPlayer::iterator pIter;
					// check if suit is equal to last winner and also number is higher
					for (pIter = activeRound.begin(); pIter != activeRound.end(); ++pIter) {
						if (*pIter != lastWinner) {
							if (lastWinner->lastStack()->cmpSuitTo((*pIter)->lastStack()) && ((*pIter)->lastStack() > lastWinner->lastStack())) {
								lastWinner = *pIter;
							} else if (turns == 4) {
								// last turn of small round -> player lost this round
								(*pIter)->lose();
							}
						}
					}
					// it's last winner's turn
					setTurn(lastWinner);
					// it's now the player's turn who has won
					if (turns == 4) {
						endSmallRound();
					} else {
						// next turn
						turns++;
					}
					
				}
				notifyAction("turn", *turn);
				return true;
			}
			break;
		}
			
		// knock
		case 3: {
			if (tPlayer->knock()) {
				activeKnock = activeRound;
				removePlayer(activeKnock, tPlayer);
				knocks++;
				notifyAction("knocked", tPlayer);
				return true;
			}
			break;
		}
			
		// call
		case 4: {
			// check if there is a knock to call and the player didn't do so already
			if (!activeKnock.empty() && find(activeKnock.begin(), activeKnock.end(), tPlayer) != activeKnock.end()) {
				removePlayer(activeKnock, tPlayer);
				tPlayer->call();
				notifyAction("called", tPlayer);
				// all players have called ?
				if (activeKnock.empty()) {
					// notify that the last knocking player can now take his turn
					notifyAction("turn", *turn);
				}
				return true;
			}
			break;
		}
			
			
	}
	
	// not executed -> may not be permitted or possible
	return false;
	
}

// notify a players or queue action for next request
void Game::notifyAction(string action, Player *aPlayer, string content) {
	Action *newAction = new Action(action, aPlayer, content);
	// queue action
	actions.push_back(newAction);
}
void Game::notifyAction(string action, Player *aPlayer) {
	Action *newAction = new Action(action, aPlayer, "");
	// queue action
	actions.push_back(newAction);
}

// get all actions since the given start up to the most recent
pair<vector<Action*>, int> Game::getActionsSince(int playerId, int start) {
	vector<Action*> newActions;
	int i = 0;
	
	// get Player object
	Player* tPlayer = getPlayer(playerId);
	if (start >= 0 && tPlayer) {
		// check if the start is higher than possible
		if (start > actions.size()) {
			start = actions.size();
		}
		
		// copy all the actions in the specific range into the new var
		vector<Action*>::iterator aIter;
		for (aIter = actions.begin()+start; aIter != actions.end(); ++aIter) {
			// hide other players' cards
			if ((*aIter)->action != "giveCards" || ((*aIter)->action == "giveCards" && (*aIter)->player == tPlayer)) {
				newActions.push_back(actions[start+i]);
			}
			i++;
		}
		
	}
	
	return pair<vector<Action*>, int>(newActions, start+i);
}

// authenticate player
// TODO: access violation when trying to authenticate before games was started
bool Game::authenticate(int playerId, string authcode) {
	// check if player is present and authentication succeeded
	if (players.count(playerId) == 1 && players[playerId]->authenticate(playerId, authcode)) {
		return true;
	}
	return false;
}

// give all players new randomly selected cards
void Game::giveCards() {
	// shuffle cards
	srand(time(0));
	random_shuffle(allCards.begin(), allCards.end());
	vector<Card*>::iterator randomCard = allCards.begin();
	vPlayer::iterator pIter;
	string givenCards;
	
	// go through all players
	for (pIter = activeRound.begin(); pIter != activeRound.end(); ++pIter) {
		givenCards = "";
		for (int i = 0; i < 4; i++) {
			// give card
			(*pIter)->giveCard(*randomCard);
			givenCards += (*randomCard)->getCardId() + ",";
			// next random card
			randomCard++;
		}
		// erase last comma
		givenCards.erase(givenCards.size()-1, 1);
		notifyAction("giveCards", *pIter, givenCards);
	}
}

// it's the next player's turn
void Game::nextTurn() {
	if (++turn == activeRound.end()) {
		turn = activeRound.begin();
	}
}

// sets the turn to the given player
void Game::setTurn(Player *tPlayer) {
	turn = find(activeRound.begin(), activeRound.end(), tPlayer);
}

// returns reference to Player object for given id
Player* Game::getPlayer(int PlayerId) {
	// check if there is a player with this id and authentication succeeded
	if (players.count(PlayerId) == 1) {
		// return Player object
		return players[PlayerId];
	}
	// not found
	return false;
}

// remove player from vector
bool Game::removePlayer(vPlayer &oPlayers, Player *delPlayer) {
	vPlayer::iterator dPlayer = find(oPlayers.begin(), oPlayers.end(), delPlayer);
	if (dPlayer != oPlayers.end()) {
		oPlayers.erase(dPlayer);
		return true;
	}
	return false;
}