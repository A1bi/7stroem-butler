#include <string>
#include <map>
#include <vector>
using namespace std;
#include "game.h"

// TODO: Exceptions und Fehlerausgabe als JSON!!

// start the game
void Game::start() {
	// initial turn
	lastWinner = *activeRound.begin();
	turn = activeRound.begin();
	// notify game has started
	notifyAction("started", *turn);
	// start first round
	startRound();
	// notify first turn
	notifyAction("turn", *turn);
}

// start a round
void Game::startRound() {
	smallRounds = 1;
	// notify that round has started
	notifyAction("startround", lastWinner);
	giveCards();
}

// end a round
void Game::endRound(Player *winner) {
	winner->win();
	notifyAction("endround", winner);
	// before we reset we have to save the current turn's pointer so we can find the player afterwards
	Player* oldTurn = *turn;
	// reset to all players active active
	activeRound.clear();
	map<int, Player*>::iterator pIter;
	for (pIter = players.begin(); pIter != players.end(); ++pIter) {
		activeRound.push_back(pIter->second);
	}
	// since we recreated activeRound we now have to find back the turn we had before using oldTurn
	// also increase to get next turn
	turn = find(activeRound.begin(), activeRound.end(), oldTurn) + 1;
	startRound();
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
	notifyAction("playerJoined", newPlayer, "");
	
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
	
	bool success = false;
	switch (possibleActions[action]) {
			
		// fold
		case 1: {
			// fold cards -> quit round
			if (tPlayer->fold()) {
				notifyAction(action, tPlayer);
				// remove player if there is an active knock
				if (!activeKnock.empty()) {
					removePlayer(activeKnock, tPlayer);
				}
				
				// remove player from active list
				removePlayer(activeRound, tPlayer);
				if (activeRound.size() == 1) {
					// only one player left -> end round
					setTurn(activeRound.front());
					endRound(activeRound.front());
				}
				notifyAction("turn", *turn);
				
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
				notifyAction("layStack", tPlayer, lCard->getCardId());
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
							} else if (smallRounds == 4) {
								// last small round -> player lost this round
								(*pIter)->lose();
							}
						}
					}
					// it's last winner's turn
					setTurn(lastWinner);
					// it's now the player's turn who has won
					if (smallRounds == 4) {
						endRound(lastWinner);
					} else {
						// next small round
						smallRounds++;
					}
				}
				notifyAction("turn", *turn);
				return true;
			}
			break;
		}
			
		// knock
		case 3: {
			success = tPlayer->knock();
			if (success) {
				activeKnock = activeRound;
				removePlayer(activeKnock, tPlayer);
				knocks++;
			}
			break;
		}
			
		// call
		case 4: {
			if (!activeKnock.empty()) {
				removePlayer(activeKnock, tPlayer);
				tPlayer->call();
				success = true;
			}
			break;
		}

			
	}
	
	// action successfully executed
	if (success) {
		notifyAction(action, tPlayer, content);
		return true;
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
	turn++;
	if (turn == activeRound.end()) {
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