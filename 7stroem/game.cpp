#include <sstream>
#include <iostream>
using namespace std;

#include "game.h"
#include "server.h"


// constructor
Game::Game(int i, int h, Server* s): gameId(i), host(h), wAPI(this), server(s), lastNotified(0) {
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
	finished = false;
	rounds = 0;
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
	// notify web server
	wAPI.startGame();
	
	// get player order for notification
	stringstream pIds;
	for (vpPlayer::iterator pIter = players.begin(); pIter != players.end(); pIter++) {
		if (pIter != players.begin()) {
			pIds << ",";
		}
		pIds << pIter->second->getId();
	}
	
	// notify players
	notifyAction("started", PlayerPtr(), pIds.str());
	
	// start first round
	startRound();
}

// destroy the game
void Game::kill() {
	server->removeGame(gameId);
}

// start a round
void Game::startRound() {
	// add all players to playersRound
	for (vpPlayer::iterator pIter = players.begin(); pIter != players.end(); ++pIter) {
		pIter->second->newRound();
		playersRound.push_back(pIter->second);
	}
	// first round -> first player is next
	if (++rounds < 2) {
		turn = playersRound.end()-1;
	}
	
	// set knock turn to a dummy value to prevent errors later
	knockTurn = playersRound.begin();
	roundStarted = true;
	origPlayers = playersRound.size();
	// web server
	wAPI.roundStarted();
	// notify that round has started
	notifyAction("roundStarted");
	startSmallRound();
}

// end a round
void Game::endRound() {
	roundStarted = false;
	// the only player left in round is the winner
	PlayerPtr winner = playersRound.front();
	// web server
	wAPI.roundEnded(winner, origPlayers);
	notifyAction("roundEnded", winner);
	// reset to all players active
	playersRound.clear();
}

// start a round
void Game::startSmallRound() {
	// add all players in this round to playersSmallRound
	someonePoor = false;
	PlayerPtr oldTurn = *turn;
	for (vPlayer::iterator pIter = playersRound.begin(); pIter != playersRound.end(); ++pIter) {
		(*pIter)->newSmallRound();
		// player is poor ?
		if ((*pIter)->getStrikes() > 5) {
			someonePoor = true;
			notifyAction("poor", *pIter);
		}
		playersSmallRound.push_back(*pIter);
		// update turn iterator position
		if (*pIter == oldTurn) {
			turn = playersSmallRound.end()-1;
		}
	}

	// if anyone is poor we have to open a knock and put all player in it who are not poor
	if (someonePoor) {
		for (vPlayer::iterator pIter = playersRound.begin(); pIter != playersRound.end(); ++pIter) {
			if ((*pIter)->getStrikes() < 6) {
				activeKnock.push_back(*pIter);
			}
		}
		knocks = 1;
		knockTurn = activeKnock.begin();
	}
	
	// increase to get next turn
	nextTurn();
	
	turns = 1;
	cardsLaid = 0;
	blindKnocked = false;
	// new winner is now the player who is first
	lastWinner = *turn;
	// notify that small round has started
	notifyAction("smallRoundStarted");
	giveCards();
	// notify first turn
	notifyTurn();
}

// end a round
void Game::endSmallRound() {
	notifyAction("smallRoundEnded", lastWinner);
	// reset to all players active
	playersSmallRound.clear();
	
	for (vPlayer::iterator pIter = playersRound.begin(); pIter != playersRound.end(); ++pIter) {
		// notify new strike state
		notifyAction("strikes", *pIter, (*pIter)->getStrikes());
		
		// player has 7 strikes -> out
		if ((*pIter)->getStrikes() > 6) {
			notifyAction("out", *pIter);
			// remove from round
			pIter = playersRound.erase(pIter)-1;
		}
	}
	
	if (playersRound.size() < 2) {
		endRound();
	} else {
		startSmallRound();
	}
	
}

// adds player to players list
bool Game::addPlayer(int playerId, string authcode) {
	// check if player not yet added
	if (getPlayer(playerId) != NULL) {
		return false;
	}
	// create pointer to new Player object
	PlayerPtr newPlayer = PlayerPtr(new Player(playerId, authcode, this, server->getIO()));
	// insert into vector
	players.push_back(pPlayer(playerId, newPlayer));
	// register as action
	notifyAction("playerJoined", newPlayer);
	
	return true;
}

// removes player from game
void Game::removePlayer(int id) {
	// get player
	vpPlayer::iterator pIter = find(players.begin(), players.end(), id);
	if (pIter == players.end()) return;
	
	PlayerPtr player = pIter->second;
	players.erase(pIter);
	int newPlayerCount = players.size();
	
	wAPI.playerQuit(player);
	notifyAction("playerQuit", player);
	
	if (started && !finished) {
		// whole game has only one player left
		if (newPlayerCount < 2) {
			cout << "spiel beendet" << endl;
			PlayerPtr lastPlayer = players.front().second;
			if (roundStarted) {
				// notify web server
				wAPI.roundEnded(lastPlayer, origPlayers);
			}
			finished = true;
			notifyAction("finished", lastPlayer);
		
		} else if (roundStarted) {
			// check if turns have to be rearranged
			bool newTurn = false;
			if (*turn == player) {
				nextTurn();
				newTurn = true;
			} else {
				// check if he has already laid a card
				if (player->cardsOnStack() >= turns) {
					cardsLaid--;
				}
			}

			if (!activeKnock.empty()) {
				if (*knockTurn == player) {
					newTurn = true;
					knockTurn++;
					removeFromKnock();
				} else {
					removeFromKnock(player);
				}
			}
			// remove from from round and small round
			removePlayerFromList(playersRound, player);
			removePlayerFromList(playersSmallRound, player, &turn);
			
			// round has one player left
			if (playersRound.size() < 2) {
				endRound();
				cout << "runde beendet" << endl;
			// small round has one player left
			} else if (playersSmallRound.size() < 2) {
				cout << "kleine runde beendet" << endl;
				endSmallRound();
			} else if (newTurn) {
				notifyTurn();
			}
		}

	}
	
	// last player left -> destroy it
	if (newPlayerCount < 1) {
		cout << "spiel zerstört" << endl;
		wAPI.finishGame();
		//end();
	
	} else {
		// check if host has to be changed
		if (newPlayerCount > 0 && host == player->getId() && !finished) {
			pPlayer* newHost = &(players.front());
			host = newHost->first;
			notifyAction("hostChanged", newHost->second);
			wAPI.changeHost();
		}
		server->sendToWaiting(shared_from_this());
	}
	
}

// host did something
bool Game::registerHostAction(PlayerPtr tPlayer, string action) {
	if (host != tPlayer->getId()) throw ActionExcept("you are not the host of this game");
	if (roundStarted) {
		throw ActionExcept("round has already started");
	}
	
	if (players.size() < 2) {
		throw ActionExcept("too few players");
	}
	if (action == "startGame") {
		if (started) throw ActionExcept("game is already running");
		start();
	
	} else if (action == "newRound") {
		if (!started) throw ActionExcept("game has not started yet");
		startRound();
	}
	return true;
}

// register an action
bool Game::registerAction(PlayerPtr tPlayer, string action, string content) {
	
	// chat
	if (action == "chat") {
		if (content == "") {
			throw ActionExcept("no message submitted");
		}
		notifyAction("chat", tPlayer, content);
		return true;
	}
	
	if (!roundStarted) {
		throw ActionExcept("round has not started yet");
		return false;
	}
	
	// call or fold ?
	if (action == "call" || action == "fold") {
		// check for knock turn
		if (tPlayer != *knockTurn) {
			throw ActionExcept("not your turn");
		} else if (activeKnock.empty()) {
			throw ActionExcept("there is no active knock");
		}
	
	// not the player's turn ?
	} else if (tPlayer != *turn && action != "flipHand" && action != "blindKnock") {
		throw ActionExcept("not your turn");
		return false;
		
	// open knock ?
	} else if ((action == "layStack" || action == "knock" || action == "blindKnock") && !activeKnock.empty()) {
		throw ActionExcept("there is an active knock");
		return false;
		
	// cannot knock if anyone is poor
	} else if ((action == "knock" || action == "blindKnock") && someonePoor) {
		throw ActionExcept("you cannot knock if anyone is poor");
		return false;
	}

	
	// fold
	if (action == "fold") {
		// fold cards -> quit round
		if (tPlayer->fold()) {
			notifyAction("folded", tPlayer);
			// remove player from active knock
			removeFromKnock();
			
			// remove player from active list
			removePlayerFromList(playersSmallRound, tPlayer, &turn);
			if (playersSmallRound.size() == 1) {
				// only one player left -> end round
				lastWinner = playersSmallRound.front();
				turn = playersSmallRound.begin();
				endSmallRound();
				
			} else {
				// check if he has already laid a card
				if (tPlayer->cardsOnStack() >= turns) {
					cardsLaid--;
				}
				notifyTurn();
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
			bool newTurn = true;
			// small round complete ?
			cardsLaid++;
			if (cardsLaid >= playersSmallRound.size()) {
				cardsLaid = 0;
				// check who won
				vPlayer::iterator pIter;
				for (pIter = playersSmallRound.begin(); pIter != playersSmallRound.end(); ++pIter) {
					if (*pIter == lastWinner) continue;
					// now two things have to be done to determine the winner of this small round
					// first: check if last winner has folded (rare case) -> player only has to have the correct suit
					// second: check if suit is equal to last winner and also number is higher
					if (((lastWinner->hasFolded() || lastWinner->hasQuit()) && !lastWinner->lastStack()->cmpSuitTo(lCard)) || 
						(*(*pIter)->lastStack()) > (*lastWinner->lastStack())) {
						lastWinner = *pIter;
						// it's now the player's turn who has won
						turn = pIter;
					}
				}
				// if the last winner is still the player who folded just choose the next player clockwise as winner
				if (lastWinner->hasFolded()) {
					// since turn is now the player next the player who folded we can set him as last winner
					lastWinner = *turn;
				}
				// last turn of small round
				if (turns == 4) {
					// check which players have to get strikes
					for (pIter = playersSmallRound.begin(); pIter != playersSmallRound.end(); ++pIter) {
						if (*pIter != lastWinner) {
							// player is not the winner -> he obviously lost
							(*pIter)->lose();
						}
					}
					endSmallRound();
					newTurn = false;
				} else {
					// next turn
					turns++;
				}
			
			}
			if (newTurn) {
				notifyTurn();
			}
			return true;
		}

	
	// knock
	} else if (action == "knock") {
		if (tPlayer->knock()) {
			knock(tPlayer, 1);
			notifyAction("knocked", tPlayer);
			notifyTurn(true);
			return true;
		}
		
		
	// blind knock
	} else if (action == "blindKnock") {
		const int blindKnocks = atoi(content.c_str());
		if (blindKnocks < 1) {
			throw ActionExcept("too few knocks");
		} else if (blindKnocked) {
			throw ActionExcept("someone has already knocked blindly");
		} else {
			tPlayer->blindKnock(blindKnocks);
			knock(tPlayer, blindKnocks);
			blindKnocked = true;
			notifyAction("blindKnocked", tPlayer, blindKnocks);
			notifyTurn(true);
			return true;
		}

		
	// call
	} else if (action == "call") {
		// check if the player didn't call already
		if (find(activeKnock.begin(), activeKnock.end(), tPlayer) != activeKnock.end()) {
			removeFromKnock();
			tPlayer->call(knocks);
			notifyAction("called", tPlayer);
			notifyTurn();
			return true;
		}
		throw ActionExcept("you have already called");

	
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
void Game::notifyAction(string action, PlayerPtr aPlayer, string content) {
	Action *newAction = new Action(action, aPlayer, content);
	// queue action
	actions.push_back(newAction);
}
// same for integer content
void Game::notifyAction(string action, PlayerPtr aPlayer, int content) {
	// int to string
	stringstream intContent;
	intContent << content;
	
	Action *newAction = new Action(action, aPlayer, intContent.str());
	// queue action
	actions.push_back(newAction);
}

// send the new turn
void Game::notifyTurn(bool knock) {
	// if there is an active knock send the next turn in this knock instead
	if (knock || !activeKnock.empty()) {
		notifyAction("knockTurn", *knockTurn);
	} else {
		notifyAction("turn", *turn);
	}
}

// get all actions since the given start up to the most recent
void Game::getActionsSince(pair<vector<Action*>, int>* pActions) {
	int i = 0, start = pActions->second;
	
	if (start < 0) {
		start = lastNotified;
	} else if (start > actions.size()) {
		return;
	} 
		
	// copy all the actions in the specific range into the new var
	vector<Action*>::iterator aIter;
	for (aIter = actions.begin()+start; aIter != actions.end(); ++aIter) {
		pActions->first.push_back(actions[start+i]);
		i++;
	}
	
	// update start in pair
	pActions->second = start+i;
}

// authenticate player
PlayerPtr Game::authenticate(int playerId, string authcode) {
	// get Player object
	PlayerPtr wantedPlayer = getPlayer(playerId);

	// authentication succeeded ?
	if (wantedPlayer != NULL && wantedPlayer->authenticate(playerId, authcode) && !wantedPlayer->hasQuit()) {
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

/*// sets the turn to the given player
void Game::setTurn(PlayerPtr tPlayer) {
	turn = find(playersSmallRound.begin(), playersSmallRound.end(), tPlayer);
}*/

// returns reference to Player object for given id
PlayerPtr Game::getPlayer(int playerId) {
	// check if there is a player with this id and authentication succeeded
	vpPlayer::iterator pIter = find(players.begin(), players.end(), playerId);
	if (pIter != players.end()) {
		// return Player object
		return pIter->second;
	}
	// not found
	return PlayerPtr();
}

// open an active knock
void Game::knock(PlayerPtr player, int k) {
	// copy all players to knock
	activeKnock = playersSmallRound;
	// find position of the one who knocked
	knockTurn = find(activeKnock.begin(), activeKnock.end(), player);
	// remove him because he cannot call or fold on a knock he intiated
	removeFromKnock();
	knocks = k;
}

// remove player from vector
bool Game::removePlayerFromList(vPlayer &oPlayers, PlayerPtr delPlayer, vPlayer::iterator* pTurn) {
	vPlayer::iterator dPlayer = find(oPlayers.begin(), oPlayers.end(), delPlayer);
	if (dPlayer != oPlayers.end()) {
		dPlayer = oPlayers.erase(dPlayer);
		// also update turn iterator ?
		if (pTurn != NULL && dPlayer < *pTurn) {
			--*pTurn;
		}
		return true;
	}
	return false;
}

// remove player from active knock
void Game::removeFromKnock(PlayerPtr player) {
	if (player != PlayerPtr()) {
		removePlayerFromList(activeKnock, player, &knockTurn);
		knockTurn++;
	} else {
		activeKnock.erase(knockTurn);
	}
	// by erasing him we also updated knockTurn so now we have to check if it is still correct
	if (knockTurn == activeKnock.end()) {
		knockTurn = activeKnock.begin();
	}
}

// destroys all players who have quit after each round
void Game::killQuitPlayers(vpPlayer::iterator* pIter) {
	if (pIter != NULL) {
		players.erase(*pIter);
	} else {
		for (vpPlayer::iterator pIter = players.begin(); pIter != players.end(); pIter++) {
			if (pIter->second->hasQuit()) {
				pIter = players.erase(pIter)-1;
			}
		}
	}
}

// update lastNotified to the id of the last action and clear waiting list
void Game::finishNotification() {
	requestsWaiting.clear();	
	lastNotified = actions.size();
}