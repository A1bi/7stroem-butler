#include "server.h"

#include <iostream>

const string Server::authcode = "zGLqz2QM5RGQkwld";

// starts game server
void Server::start() {
	
	// create listening sock whick checks for new connections
	Socket listeningSock;
	listeningSock.bind(4926);
	listeningSock.listen();
	// create new connection checking thread
	boost::thread listening(boost::bind(&Server::listen, this, &listeningSock));
	
	// check connections and missing players periodically
	boost::thread checkingConns(boost::bind(&Server::checkConnections, this));
	
	// don't stop here - wait for listening thread
	listening.join();
}

// listen for new connections, accept and pass them on to a new thread of handleNewConnection()
void Server::listen(Socket* sock) {
	
	// socket for a new connection
	Socket newConnSock;
	
	// TODO: check if server is not full
	while (true) {
		// accept new incoming connections
		sock->accept(newConnSock);
		
		// create new thread for each new connection
		boost::thread newConn(boost::bind(&Server::handleNewConnection, this, newConnSock.getSock()));
	}
}

// checks all open connections if they might have been closed
void Server::checkConnections() {
	
	Socket checkSock;
	int ready, sockMax;
	string dummy;
	vector<PlayerRequest*>::iterator vIter;
	fd_set sockSet;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;
	
	while (true) {
		
		sleep(3);
		
		// check if any players have to be removed
		checkMissingPlayers();
		
		FD_ZERO(&sockSet);
		sockMax = -1;
		boost::mutex::scoped_lock lock(mutexConn);
		// create a socket set which contains all open connections
		for (vIter = openConnections.begin(); vIter != openConnections.end(); ++vIter) {
			// already closed by sendActions() ?
			if ((*vIter)->sock == -2) {
				// remove from list
				delete *vIter;
				openConnections.erase(vIter);
				vIter--;
				continue;
			}
			
			FD_SET((*vIter)->sock, &sockSet);
			if ((*vIter)->sock > sockMax) {
				sockMax = (*vIter)->sock;
			}
		}
		
		// check on all sockets in socket set if there is a socket ready for reading
		ready = select(sockMax+1, &sockSet, NULL, NULL, &timeout);
		if (ready < 1) continue;

		// check all sockets to read from
		for (vIter = openConnections.begin(); vIter != openConnections.end(); ++vIter) {
			
			// socket in reading socket set -> new data to read (receive) ?
			if (FD_ISSET((*vIter)->sock, &sockSet)) {
				checkSock.setSock((*vIter)->sock);
				// try to read data
				if (checkSock.recv(dummy) == 0) {
					// connection has closed
					checkSock.close();
					// mark player as disconnected
					(*vIter)->player->setDisconnected();
					missingPlayers.push_back((*vIter)->player);
					// remove from set
					FD_CLR((*vIter)->sock, &sockSet);
					(*vIter)->sock = -1;
					openConnections.erase(vIter);
					vIter--;
					cout << "connection closed" << endl;
				}
				
				// any reading sockets left to be processed
				if (--ready <= 0) {
					break;
				}
			}
			
		}

	}
	
}

// checks for missing players and removes them from their games if neccessary
void Server::checkMissingPlayers() {
	
	vector<Player*>::iterator vIter;
	for (vIter = missingPlayers.begin(); vIter != missingPlayers.end(); ++vIter) {
		if ((*vIter)->isConnected()) {
			missingPlayers.erase(vIter);
			vIter--;
		} else if ((*vIter)->isMissing()) {
			missingPlayers.erase(vIter);
			vIter--;
			cout << "player disconnected" << endl;
		}

	}
		
}

// handles an incoming connection after being accepted by the listening socket
void Server::handleNewConnection(int sockNo) {

	Socket recvSock;
	recvSock.setSock(sockNo);
	string rawRequest;
	
	// parsing request
	recvSock.recv(rawRequest);
	HTTPrequest request(&rawRequest);
	
	// it's a player request
	if (request.getUri() == "/player") {
		handlePlayerRequest(&request, &recvSock);
	
	// it's a request by the 7stroem server
	} else if (request.getUri() == "/server") {
		HTTPresponse response;
		if (request.getGet("authcode") == authcode && handleServerRequest(&request)) {
			response << "ok";
		} else {
			response << "error";
		}
		response.send(&recvSock);
		recvSock.close();
		
	// unknown action
	} else {
		recvSock.send("unknown action");
		recvSock.close();
	}
	
}

// handles requests from players
void Server::handlePlayerRequest(HTTPrequest* request, Socket* sock) {
	
	Game* myGame;
	int playerId = atoi(request->getGet("pId").c_str());
	int gameId = atoi(request->getGet("gId").c_str());
	
	try {
		
		// get Game object
		// check if there is a player with this id and authentication succeeded
		if (games.count(gameId) == 1) {
			myGame = games[gameId];
		} else {
			// game not found
			throw "game not found";
		}
		
		// lock this game
		boost::mutex::scoped_lock lock(myGame->mutex);
		
		// authenticate player and get its object
		Player* tPlayer = myGame->authenticate(playerId, request->getGet("authcode"));
		if (tPlayer != NULL) {
			
			// responding to game request
			string gameRequest = request->getGet("request");
			
			// player wants to get actions
			if (gameRequest == "getActions") {
				
				// mark player as connected
				tPlayer->setConnected();
				
				// get since argument and check if not empty
				int since = atoi(request->getGet("since").c_str());
				
				// create request object
				PlayerRequest* newRequest = new PlayerRequest(myGame, tPlayer, since, sock->getSock());
				
				// send actions if there are already new actions
				if (!sendActions(newRequest)) {
					// put player into waiting list and send actions later when they occur
					myGame->requestsWaiting.push_back(newRequest);
					boost::mutex::scoped_lock lock(mutexConn);
					openConnections.push_back(newRequest);
					
				// actions already sent -> destroy request
				} else {
					delete newRequest;
				}
				
				
			// player performed an action
			} else if (gameRequest == "registerAction") {
				// register action
				myGame->registerAction(tPlayer, request->getGet("action"), request->getGet("content"));
				
				// success
				JSONobject jsonResponse;
				// prepare json response and add to body
				jsonResponse.addChild("result", "ok");
				// if player flipped his hand we now have to send him his cards so he can see them
				if (request->getGet("action") == "flipHand") {
					JSONarray* jsonCards = jsonResponse.addArray("cards");
					
					string cards[4];
					tPlayer->getHand(cards);
					for (int i = 0; i < 4; i++) {
						jsonCards->addChild(cards[i]);
					}
				}
				
				// send response
				sendResponse(sock, &jsonResponse);
				
				if (request->getGet("action") != "flipHand") {
					// send new actions to waiting players
					sendToWaiting(myGame);
				}
				
			// unknown request -> close
			} else {
				throw "unknown request";
			}
			
		// authentication failed -> close
		} else {
			throw "authentication failed";
		}
		
	}
	// some error occurred ?
	catch (string msg) {
		sendError(sock, msg);
	}
	// action not allowed ?
	catch (ActionExcept& e) {
		sendError(sock, e.getErrorMsg(), e.getErrorId());
	}
	
}

// send all actions to waiting players
void Server::sendToWaiting(Game* game) {
	// notifying all waiting players of the new actions
	vector<PlayerRequest*>::iterator rIter;
	// make sure it doesn't interfere with checkConnections()
	boost::mutex::scoped_lock lock(mutexConn);
	for (rIter = game->requestsWaiting.begin(); rIter != game->requestsWaiting.end(); ++rIter) {
		// check if connection is not closed
		// TODO: abfolge checken
		if (*rIter != NULL && (*rIter)->sock > -1) {
			// connection still alive -> send new actions
			sendActions(*rIter);
			// remove from open connections list
			(*rIter)->sock = -2;
			
		} else {
			// destroy request
			delete *rIter;
		}

	}
	// remove everything from waiting list
	game->requestsWaiting.clear();
}

// handles requests from the remote 7stroem server
bool Server::handleServerRequest(HTTPrequest* request) {
	// create game
	if (request->getGet("request") == "createGame") {
		if (request->getGet("id") != "" && createGame(atoi(request->getGet("id").c_str()))) {
			return true;
		}
	// requests with game id
	} else if (request->getGet("gId") != "") {
		// get game
		map<int, Game*>::iterator mIter = games.find( atoi(request->getGet("gId").c_str()) );
		// check if game exists
		if (mIter != games.end()) {
			// register player
			if (request->getGet("request") == "registerPlayer") {
				if (request->getGet("pId") != "" && request->getGet("pAuthcode") != "" && mIter->second->addPlayer( atoi(request->getGet("pId").c_str()), request->getGet("pAuthcode") )) {
					sendToWaiting(mIter->second);
					return true;
				}
			// start game
			} else if (request->getGet("request") == "startGame") {
				mIter->second->start();
				sendToWaiting(mIter->second);
				return true;
			}
		}
	}
	return false;
}

// send actions to players in waiting list
bool Server::sendActions(PlayerRequest* request) {
	
	vector<Action*>::iterator aIter;
	vector<Action*> *newActions;
	pair<vector<Action*>, int> actions;
	
	// get all actions since given event number
	actions = request->game->getActionsSince(request->player, request->since);
	// no new actions ?
	if (actions.second - request->since < 1) {
		// stop here - request gets on waiting list
		return false;
	}
	
	// prepare response
	JSONobject jsonResponse;
	jsonResponse.addChild("result", "ok");
	JSONarray* actionsArray = jsonResponse.addArray("actions");
	
	// go through all actions
	newActions = &(actions.first);
	for (aIter = newActions->begin(); aIter != newActions->end(); ++aIter) {
		JSONobject* action = actionsArray->addObject();
		action->addChild("action", (*aIter)->action);
		if ((*aIter)->player != NULL) {
			stringstream pId;
			pId << (*aIter)->player->getId();
			action->addChild("player", pId.str());
		}
		action->addChild("content", (*aIter)->content);
	}
	
	// conclude response with the number of the last action
	stringstream lastAction;
	lastAction << actions.second;
	jsonResponse.addChild("lastAction", lastAction.str());
	
	// send response to player
	Socket actionsSock;
	actionsSock.setSock(request->sock);
	sendResponse(&actionsSock, &jsonResponse);
	
	// mark this player's client as disconnected
	request->player->setDisconnected();
	
	return true;
	
}

// creates a new game and stores it under given id
bool Server::createGame(int gameId) {
	// check if game id not yet created
	if (games.find(gameId) == games.end()) {
		Game* newGame = new Game;
		games[gameId] = newGame;
		return true;
	}
	return false;
}

// send a response as a JSON object to player
void Server::sendResponse(Socket* sock, JSONobject* response) {
	// prepare reponse
	HTTPresponse httpResponse;
	httpResponse << "game.processResponse(" + response->str() + ");";
	// send response to player
	httpResponse.send(sock);
	
	// close and cleanup everything
	sock->close();
}

// send error back to player
void Server::sendError(Socket* sock, string msg, string id) {
	
	// success
	JSONobject jsonResponse;
	// prepare json response and add to body
	jsonResponse.addChild("result", "error");
	JSONobject* error = jsonResponse.addObject("error");
	error->addChild("id", id);
	error->addChild("message", msg);
	
	// send to player
	sendResponse(sock, &jsonResponse);
}