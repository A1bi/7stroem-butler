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
			// first check if pointers are still valid (may have been deleted after a player was removed)
			if ((*vIter)->gameCon == NULL || (*vIter)->player == NULL) {
				vIter = openConnections.erase(vIter)-1;
				continue;
			}
			
			// add to socket set
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
					missingPlayers.push_back(*vIter);
					// remove from set
					FD_CLR((*vIter)->sock, &sockSet);
					// remove from request list
					(*vIter)->gameCon->requestsWaiting.erase(find((*vIter)->gameCon->requestsWaiting.begin(), (*vIter)->gameCon->requestsWaiting.end(), *vIter));
					// remove from open connections list
					vIter = openConnections.erase(vIter)-1;
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
	
	vector<GameContainer*> gamesRemoved;
	vector<PlayerRequest*>::iterator vIter;
	for (vIter = missingPlayers.begin(); vIter != missingPlayers.end(); ++vIter) {
		
		// is completely disconnected
		Player* player = (*vIter)->player;
		GameContainer* gameCon = (*vIter)->gameCon;
		// check if this games has not yet been removed
		if (find(gamesRemoved.begin(), gamesRemoved.end(), gameCon) == gamesRemoved.end()) {
			
			cout << player << " check" << endl;
			if (!player->isConnected()) {
				if (player->isMissing()) {
					cout << "player disconnected" << endl;
					Game* game = gameCon->game;
					cout << game << endl;
					
					// lock mutex
					boost::mutex::scoped_lock lock(gameCon->mutex);
					
					// get number of players to determine if any player has to be notified of the finished game
					if (game->removePlayer(player)) {
						// no players left -> remove game
						games.erase(game->getId());
						gamesRemoved.push_back(gameCon);
						sendToWaiting(gameCon);
						
						// unlock mutex before destroying the object
						lock.unlock();
						// destroy object
						delete gameCon;
					} else {
						sendToWaiting(gameCon);
					}

				// if player is disconnected but not yet missing we have to skip removal of player from missing list
				} else {
					continue;
				}
			}
		
		}
		
		delete *vIter;
		vIter = missingPlayers.erase(vIter)-1;
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
	
	GameContainer* myGameCon;
	Game* myGame;
	int playerId = atoi(request->getGet("pId").c_str());
	int gameId = atoi(request->getGet("gId").c_str());
	
	try {
		
		// get Game object
		// check if there is a player with this id and authentication succeeded
		if (games.count(gameId) == 1) {
			myGameCon = games[gameId];
			myGame = myGameCon->game;
			
		} else {
			// game not found
			throw "game not found";
		}
		
		// lock this game
		boost::mutex::scoped_lock lock(myGameCon->mutex);
		
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
				PlayerRequest* newRequest = new PlayerRequest(myGameCon, tPlayer, since, sock->getSock());
				
				// send actions if there are already new actions
				if (!sendActions(newRequest)) {
					boost::mutex::scoped_lock lock(mutexConn);
					// put player into waiting list and send actions later when they occur
					myGameCon->requestsWaiting.push_back(newRequest);
					openConnections.push_back(newRequest);
					
				// actions already sent -> destroy request
				} else {
					delete newRequest;
				}
				
				
			// player performed an action
			} else if (gameRequest == "registerAction" || gameRequest == "registerHostAction") {
				// success
				JSONobject jsonResponse;
				// prepare json response and add to body
				jsonResponse.addChild("result", "ok");
				
				if (gameRequest == "registerAction") {
					// register action
					myGame->registerAction(tPlayer, request->getGet("action"), request->getGet("content"));
					
					// if player flipped his hand we now have to send him his cards so he can see them
					if (request->getGet("action") == "flipHand") {
						JSONarray* jsonCards = jsonResponse.addArray("cards");
						
						string cards[4];
						tPlayer->getHand(cards);
						for (int i = 0; i < 4; i++) {
							jsonCards->addChild(cards[i]);
						}
					}
					
					if (request->getGet("action") != "flipHand") {
						// send new actions to waiting players
						sendToWaiting(myGameCon);
					}
				
				} else if (gameRequest == "registerHostAction") {
					myGame->registerHostAction(tPlayer, request->getGet("action"));
					sendToWaiting(myGameCon);
				}
				
				// send response
				sendResponse(sock, &jsonResponse);
				
				
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
	catch (char* msg) {
		sendError(sock, string(msg));
	}
	// action not allowed ?
	catch (ActionExcept& e) {
		sendError(sock, e.getErrorMsg(), e.getErrorId());
	}
	
}

// send all actions to waiting players
void Server::sendToWaiting(GameContainer* gameCon) {
	// notifying all waiting players of the new actions
	vector<PlayerRequest*>::iterator rIter;
	// make sure it doesn't interfere with checkConnections()
	boost::mutex::scoped_lock lock(mutexConn);
	for (rIter = gameCon->requestsWaiting.begin(); rIter != gameCon->requestsWaiting.end(); ++rIter) {
		// send new actions
		sendActions(*rIter);
		// remove from open connections list
		openConnections.erase(find(openConnections.begin(), openConnections.end(), *rIter));
		// destroy request
		delete *rIter;
	}
	// remove everything from waiting list
	gameCon->requestsWaiting.clear();
}

// handles requests from the remote 7stroem server
bool Server::handleServerRequest(HTTPrequest* request) {
	// create game
	if (request->getGet("request") == "createGame") {
		if (request->getGet("id") != "" && request->getGet("host") != "" && createGame(atoi(request->getGet("id").c_str()), atoi(request->getGet("host").c_str()))) {
			return true;
		}
	// requests with game id
	} else if (request->getGet("gId") != "") {
		// get game
		GameContainer* gameCon = games[ atoi(request->getGet("gId").c_str()) ];
		// check if game exists
		if (gameCon != NULL) {
			// register player
			if (request->getGet("request") == "registerPlayer") {
				if (request->getGet("pId") != "" && request->getGet("pAuthcode") != "") {
					Player* newPlayer = gameCon->game->addPlayer( atoi(request->getGet("pId").c_str()), request->getGet("pAuthcode") );
					if (newPlayer == NULL) {
						return false;
					}
					// add to missing list - the player might never connect in the first place
					boost::mutex::scoped_lock lock(mutexConn);
					// push back a dummy player request
					missingPlayers.push_back(new PlayerRequest(gameCon, newPlayer, -1, -1));
					lock.unlock();
					sendToWaiting(gameCon);
					return true;
				}
			/*
			// start game - obsolete
			} else if (request->getGet("request") == "startGame") {
				gameCon->game->start();
				sendToWaiting(gameCon);
				return true;
			*/
			}
		}
	}
	return false;
}

// send actions to players in waiting list
bool Server::sendActions(PlayerRequest* request) {
	
	pair<vector<Action*>, int> actions;
	actions.second = request->since;
	
	// get all actions since given event number
	request->gameCon->game->getActionsSince(&actions);
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
	for (vector<Action*>::iterator aIter = actions.first.begin(); aIter != actions.first.end(); ++aIter) {
		JSONobject* action = actionsArray->addObject();
		action->addChild("action", (*aIter)->action);
		stringstream pId;
		pId << (*aIter)->player;
		action->addChild("player", pId.str());
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
bool Server::createGame(int gameId, int host) {
	// check if game id not yet created
	if (games.find(gameId) == games.end()) {
		games[gameId] = new GameContainer(gameId, host);
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