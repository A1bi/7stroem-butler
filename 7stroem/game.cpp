#include <string>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
#include "game.h"
#include <iostream>
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
	possibleActions["flipHand"] = 5;
}

// start the game
void Game::start() {
	// notify game has started
	notifyAction("started");
	// start first round
	startRound();
}

// start a round
void Game::startRound() {
	// add all players to playersRound
	for (map<int, Player*>::iterator pIter = players.begin(); pIter != players.end(); ++pIter) {
		playersRound.push_back(pIter->second);
	}
	// initial turn
	turn = playersRound.end()-1;
	roundStarted = true;
	// notify that round has started
	notifyAction("roundStarted");
	startSmallRound();
}

// start a round
void Game::startSmallRound() {
	// before we reset we have to save the current turn's pointer so we can find the player afterwards
	Player* oldTurn = *turn;
	// add all players in this round to playersSmallRound
	for (vPlayer::iterator pIter = playersRound.begin(); pIter != playersRound.end(); ++pIter) {
		(*pIter)->newRound();
		if ((*pIter)->getStrikes() > 5) {
			notifyAction("poor", *pIter);
		}
		playersSmallRound.push_back(*pIter);
	}
	
	// since we recreated playersSmallRound we now have to find back the turn we had before using oldTurn
	// also increase to get next turn
	turn = find(playersSmallRound.begin(), playersSmallRound.end(), oldTurn) + 1;
	if (turn == playersSmallRound.end()) {
		turn = playersSmallRound.begin();
	}
	
	turns = 1;
	// new winner is now the player who is first
	lastWinner = *turn;
	// notify that small round has started
	notifyAction("smallRoundStarted");
	giveCards();
	// notify first turn
	notifyAction("turn", *turn);
}

// end a round
void Game::endSmallRound() {
	notifyAction("smallRoundEnded", lastWinner);
	// reset to all players active
	playersSmallRound.clear();
	stringstream strikes;
	
	for (map<int, Player*>::iterator pIter = players.begin(); pIter != players.end(); ++pIter) {
		// notify new strike state
		strikes << pIter->second->getStrikes();
		notifyAction("strikes", pIter->second, strikes.str());
		strikes.str("");
		strikes.clear();
		
		// player has 7 strikes -> out
		if (pIter->second->getStrikes() > 6) {
			notifyAction("out", pIter->second);
			// remove from round
			playersRound.erase(find(playersRound.begin(), playersRound.end(), pIter->second));
		}
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
	// register as action
	notifyAction("playerJoined", newPlayer);
	
	return true;
}

// register an action
bool Game::registerAction(Player* tPlayer, string action, string content) {
	
	// chat
	if (action == "chat") {
		notifyAction("chat", tPlayer, content);
		return true;
	}
	
	// TODO: FIX: bad access vor spielbeginn
	// not the player's turn ?
	if (tPlayer != *turn && action != "flipHand") {
		throw ActionExcept("not your turn");
		return false;
	}
	
	switch (possibleActions[action]) {
			
		// fold
		case 1: {
			// folding is only possible in an active knock
			if (activeKnock.empty()) {
				throw ActionExcept("no active knock");
			}
			// fold cards -> quit round
			if (tPlayer->fold()) {
				notifyAction("folded", tPlayer);
				// remove player from active knock
				removePlayer(activeKnock, tPlayer);
				
				// save current turn for later
				Player* oldTurn = *turn;
				// remove player from active list
				removePlayer(playersSmallRound, tPlayer);
				if (playersSmallRound.size() == 1) {
					// only one player left -> end round
					lastWinner = playersSmallRound.front();
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
						if (turn == playersSmallRound.end()) {
							lastWinner = playersSmallRound.front();
						} else {
							lastWinner = *turn;
						}
					}
				}
				
				return true;
			}
			// error -> already folded
			throw ActionExcept("already folded");
			break;
		}
			
		// layStack
		case 2: {
			// get Card object
			Card* lCard = tPlayer->cardFromHand(content);
			if (!lCard) {
				throw ActionExcept("card not in your hand");
				return false;
			}
			
			// check if player laid a suit different to the leading although he has the correct suit in hand -> not permitted
			if (tPlayer != lastWinner && !lastWinner->lastStack()->cmpSuitTo(lCard) && tPlayer->checkForSuit(lastWinner->lastStack())) {
				throw ActionExcept("you got the right suit, admit it!", "admit");
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
					for (pIter = playersSmallRound.begin(); pIter != playersSmallRound.end(); ++pIter) {
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
				activeKnock = playersSmallRound;
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
			
		// flipped hand
		case 5: {
			tPlayer->flipHand();
			return true;
			break;
		}
			
			
	}
	
	// not executed -> may not be permitted or possible
	throw ActionExcept("unknown error");
	return false;
	
}

// notify a players or queue action for next request
void Game::notifyAction(string action, Player *aPlayer, string content) {
	Action *newAction = new Action(action, aPlayer, content);
	// queue action
	actions.push_back(newAction);
}
// TODO: überdenken: alle spieler kriegen ab sofort die gleichen actions geliefert, nur einmal also abrufen ?
// get all actions since the given start up to the most recent
pair<vector<Action*>, int> Game::getActionsSince(Player* tPlayer, int start) {
	vector<Action*> newActions;
	int i = 0;
	
	if (start >= 0 && tPlayer) {
		// check if the start is higher than possible
		if (start > actions.size()) {
			start = actions.size();
		}
		
		// copy all the actions in the specific range into the new var
		vector<Action*>::iterator aIter;
		for (aIter = actions.begin()+start; aIter != actions.end(); ++aIter) {
			newActions.push_back(actions[start+i]);
			i++;
		}
		
	}
	
	return pair<vector<Action*>, int>(newActions, start+i);
}

// authenticate player
// TODO: access violation when trying to authenticate before games was started
Player* Game::authenticate(int playerId, string authcode) {
	// get Player object
	Player* wantedPlayer = getPlayer(playerId);
	// authentication succeeded ?
	if (wantedPlayer->authenticate(playerId, authcode)) {
		return wantedPlayer;
	}
	return NULL;
}

// give all players new randomly selected cards
void Game::giveCards() {
	// shuffle cards
	srand(time(0));
	random_shuffle(allCards.begin(), allCards.end());
	vector<Card*>::iterator randomCard = allCards.begin();
	vPlayer::iterator pIter;
	
	// go through all players
	for (pIter = playersRound.begin(); pIter != playersRound.end(); ++pIter) {
		for (int i = 0; i < 4; i++) {
			// give card
			(*pIter)->giveCard(*randomCard);
			// next random card
			randomCard++;
		}
	}
}

// it's the next player's turn
void Game::nextTurn() {
	if (++turn == playersSmallRound.end()) {
		turn = playersSmallRound.begin();
	}
}

// sets the turn to the given player
void Game::setTurn(Player *tPlayer) {
	turn = find(playersSmallRound.begin(), playersSmallRound.end(), tPlayer);
}

// returns reference to Player object for given id
Player* Game::getPlayer(int PlayerId) {
	// check if there is a player with this id and authentication succeeded
	if (players.count(PlayerId) == 1) {
		// return Player object
		return players[PlayerId];
	}
	// not found
	return NULL;
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