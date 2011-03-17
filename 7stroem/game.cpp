#include <string>
#include <sstream>
#include <map>
#include <vector>
using namespace std;
#include "game.h"
#include <iostream>

// constructor
Game::Game(int i): gameId(i) {
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
	roundStarted = false;
	started = false;
}

// destructor
Game::~Game() {
	// destroy all cards
	for (vector<Card*>::iterator vIter = allCards.begin(); vIter != allCards.end(); ++vIter) {
		delete *vIter;
	}
	// destroy all actions
	for (vector<Action*>::iterator vIter = actions.begin(); vIter != actions.end(); ++vIter) {
		delete *vIter;
	}
}

// just return the id of this game
int Game::getId() {
	return gameId;
}

// start the game
void Game::start() {
	started = true;
	// notify game has started
	notifyAction("started");
	// start first round
	startRound();
}

// start a round
void Game::startRound() {
	// add all players to playersRound
	for (vpPlayer::iterator pIter = players.begin(); pIter != players.end(); ++pIter) {
		pIter->second->newRound();
		playersRound.push_back(pIter->second);
	}
	// initial turn
	turn = playersRound.end()-1;
	roundStarted = true;
	// notify that round has started
	notifyAction("roundStarted");
	startSmallRound();
}

// end a round
void Game::endRound() {
	// TODO: update credit
	roundStarted = false;
	// the only player left in round is the winner
	Player* winner = playersRound.front();
	notifyAction("roundEnded", winner);
}

// start a round
void Game::startSmallRound() {
	// before we reset we have to save the current turn's pointer so we can find the player afterwards
	Player* oldTurn = *turn;
	// add all players in this round to playersSmallRound
	for (vPlayer::iterator pIter = playersRound.begin(); pIter != playersRound.end(); ++pIter) {
		(*pIter)->newSmallRound();
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
	
	for (vpPlayer::iterator pIter = players.begin(); pIter != players.end(); ++pIter) {
		// notify new strike state
		notifyAction("strikes", pIter->second, pIter->second->getStrikes());
		
		// player has 7 strikes -> out
		if (pIter->second->getStrikes() > 6) {
			notifyAction("out", pIter->second);
			// remove from round
			playersRound.erase(find(playersRound.begin(), playersRound.end(), pIter->second));
		}
	}
	
	if (playersRound.size() < 2) {
		endRound();
	} else {
		startSmallRound();
	}
	
}

// adds player to players list
Player* Game::addPlayer(int playerId, string authcode) {
	// check if player not yet added
	if (getPlayer(playerId) != NULL) {
		return NULL;
	}
	// create pointer to new Player object
	Player *newPlayer = new Player(playerId, authcode, this);
	// insert into vector
	players.push_back(pPlayer(playerId, newPlayer));
	// register as action
	notifyAction("playerJoined", newPlayer);
	
	return newPlayer;
}

// removes player from game
bool Game::removePlayer(Player* player) {
	// TODO: change host player
	bool destroyGame = false;
	vpPlayer::iterator pIter = find(players.begin(), players.end(), player->getId());
	if (pIter == players.end()) {
		// player not found
		return false;
	}
	notifyAction("playerQuit", NULL, player->getId());
	players.erase(pIter);
	if (roundStarted) {
		bool newTurn = false;
		if (*turn == player) {
			nextTurn();
			newTurn = true;
		}
		if (lastWinner == player) {
			if (turn == playersSmallRound.end()) {
				lastWinner = playersSmallRound.front();
			} else {
				lastWinner = *turn;
			}
		}
		playersRound.erase(find(playersRound.begin(), playersRound.end(), player));
		playersSmallRound.erase(find(playersSmallRound.begin(), playersSmallRound.end(), player));
		// whole game has only one player left
		if (players.size() < 2) {
			// TODO: update player credit
			cout << "spiel beendet" << endl;
			notifyAction("finished", player);
		// round has one player left
		} else if (playersRound.size() < 2) {
			endRound();
			cout << "runde beendet" << endl;
		// small round has one player left
		} else if (playersSmallRound.size() < 2) {
			cout << "kleine runde beendet" << endl;
			endSmallRound();
		} else if (newTurn) {
			notifyAction("turn", *turn);
		}

	}
	// last player left -> destroy it
	if (players.size() < 1) {
		cout << "spiel zerstört" << endl;
		destroyGame = true;
	}
	delete player;
	// returns true if the game has to be removed from game list in server.cpp
	return destroyGame;
}

// register an action
bool Game::registerAction(Player* tPlayer, string action, string content) {
	
	// chat
	if (action == "chat") {
		notifyAction("chat", tPlayer, content);
		return true;
	}
	
	// not the player's turn ?
	if (tPlayer != *turn && action != "flipHand") {
		throw ActionExcept("not your turn");
		return false;
	}
	
	// open knock ?
	if ((action == "layStack" || action == "knock") && !activeKnock.empty()) {
		throw ActionExcept("there is an active knock");
		return false;
	}
	
	
	// fold
	if (action == "fold") {
		// folding is only possible in an active knock
		if (activeKnock.empty()) {
			throw ActionExcept("no active knock");
		}
		// fold cards -> quit round
		if (tPlayer->fold()) {
			notifyAction("folded", tPlayer);
			// remove player from active knock
			removePlayerFromList(activeKnock, tPlayer);
			
			nextTurn();
			// save current turn for later
			Player* oldTurn = *turn;
			// remove player from active list
			removePlayerFromList(playersSmallRound, tPlayer);
			if (playersSmallRound.size() == 1) {
				// only one player left -> end round
				lastWinner = playersSmallRound.front();
				setTurn(lastWinner);
				endSmallRound();
				
			} else {
				setTurn(oldTurn);
				notifyAction("turn", *turn);
				// last winner is now the player next to this player
				// TODO: fix this
				if (lastWinner == tPlayer) {
					if (turn+1 == playersSmallRound.end()) {
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
	
	
	// layStack
	} else if (action == "layStack") {
		// get Card object
		Card* lCard = tPlayer->cardFromHand(content);
		if (!lCard) {
			throw ActionExcept("card not in your hand");
			return false;
		}
		
		// check if player laid a suit different to the leading although he has the correct suit in hand -> not permitted
		if (tPlayer != lastWinner && !lastWinner->lastStack()->cmpSuitTo(lCard) && tPlayer->checkForSuit(lastWinner->lastStack())) {
			throw ActionExcept("you have to admit", "admit");
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
					if (*pIter == lastWinner) continue;
					if ((*(*pIter)->lastStack()) > (*lastWinner->lastStack())) {
						lastWinner = *pIter;
					} else if (turns == 4) {
						// last turn of small round -> player lost this round
						(*pIter)->lose();
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

	
	// knock
	} else if (action == "knock") {
		if (tPlayer->knock()) {
			activeKnock = playersSmallRound;
			removePlayerFromList(activeKnock, tPlayer);
			knocks++;
			notifyAction("knocked", tPlayer);
			nextTurn();
			notifyAction("turn", *turn);
			return true;
		}

	
	// call
	} else if (action == "call") {
		// check if there is a knock to call and the player didn't do so already
		if (!activeKnock.empty() && find(activeKnock.begin(), activeKnock.end(), tPlayer) != activeKnock.end()) {
			removePlayerFromList(activeKnock, tPlayer);
			tPlayer->call();
			notifyAction("called", tPlayer);
			nextTurn();
			notifyAction("turn", *turn);
			return true;
		}

	
	// flipHand
	} else if (action == "flipHand") {
		tPlayer->flipHand();
		return true;
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
// same for integer content
void Game::notifyAction(string action, Player *aPlayer, int content) {
	// int to string
	stringstream intContent;
	intContent << content;
	
	Action *newAction = new Action(action, aPlayer, intContent.str());
	// queue action
	actions.push_back(newAction);
}

// TODO: überdenken: alle spieler kriegen ab sofort die gleichen actions geliefert, nur einmal also abrufen ?
// TODO: mach den ganzen since scheiß weg!
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
Player* Game::authenticate(int playerId, string authcode) {
	// get Player object
	Player* wantedPlayer = getPlayer(playerId);

	// authentication succeeded ?
	if (wantedPlayer != NULL && wantedPlayer->authenticate(playerId, authcode)) {
		return wantedPlayer;
	}
	return wantedPlayer;
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
	if (++turn >= playersSmallRound.end()) {
		turn = playersSmallRound.begin();
	}
}

// sets the turn to the given player
void Game::setTurn(Player *tPlayer) {
	turn = find(playersSmallRound.begin(), playersSmallRound.end(), tPlayer);
}

// returns reference to Player object for given id
Player* Game::getPlayer(int playerId) {
	// check if there is a player with this id and authentication succeeded
	vpPlayer::iterator pIter = find(players.begin(), players.end(), playerId);
	if (pIter != players.end()) {
		// return Player object
		return pIter->second;
	}
	// not found
	return NULL;
}

// remove player from vector
bool Game::removePlayerFromList(vPlayer &oPlayers, Player *delPlayer) {
	vPlayer::iterator dPlayer = find(oPlayers.begin(), oPlayers.end(), delPlayer);
	if (dPlayer != oPlayers.end()) {
		oPlayers.erase(dPlayer);
		return true;
	}
	return false;
}