#include "server.h"
#include "json.h"
#include <iostream>

const string Server::authcode = "zGLqz2QM5RGQkwld";

// initiates game server
void Server::start() {
	
	int i, ready, sockMax, max = -1;
	Socket listeningSock, newConnSock, recvSock;
	listeningSock.bind(4926);
	listeningSock.listen();
	
	sockMax = listeningSock.getSock();
	for (i = 0; i < FD_SETSIZE; i++) {
		connSocks[i] = -1;
	}
	FD_ZERO(&sockSet);
	FD_SET(listeningSock.getSock(), &sockSet);
	
	
	while (true) {
		
		// reset reading socket set
		sockReadSet = sockSet;
		// check on all sockets in socket set if there is a socket ready for reading
		ready = select(sockMax+1, &sockReadSet, NULL, NULL, NULL);
		// new connection ?
		if (FD_ISSET(listeningSock.getSock(), &sockReadSet)) {
			listeningSock.accept(newConnSock);
			
			// assign new connection to a free socket number
			for (i = 0; i < FD_SETSIZE; i++) {
				if (connSocks[i] < 0) {
					connSocks[i] = newConnSock.getSock();
					break;
				}
			}
			
			// no free number - full
			if (i == FD_SETSIZE) {
				newConnSock.close();
				//throw SockExcept("too many clients. haltung!");
				continue;
			}
			
			// add connection to socket set
			FD_SET(newConnSock.getSock(), &sockSet);
			// get highest socket number for select()
			if (newConnSock.getSock() > sockMax) {
				sockMax = newConnSock.getSock();
			}
			// get highest sock number - needed later for checking all the reading sockets
			if (i > max) {
				max = i;
			}
			// if there is no other ready socket we can stop here
			if (--ready <= 0) {
				continue;
			}
		}
		
		// check all sockets to read from
		// go through all sockets up to max
		for (i = 0; i <= max; i++) {
			string rawRequest;
			
			if (connSocks[i] < 0) {
				continue;
			}
			recvSock.setSock(connSocks[i]);
			
			// socket in reading socket set -> new data to read (receive) ?
			if (FD_ISSET(connSocks[i], &sockReadSet)) {
				// receive data - getting request
				if (recvSock.recv(rawRequest) == 0) {
					// connection closed
					closeConn(&recvSock);
					// check if socket was waiting for actions
					map<int, PlayerRequest*>::iterator rIter = requestsWaitingSocks.find(connSocks[i]);
					if (rIter != requestsWaitingSocks.end()) {
						// mark as closed so it will get destroyed later when handlePlayerRequest() tries to send the new actions
						rIter->second->sock = -1;
						// remove
						requestsWaitingSocks.erase(rIter);
					}
				}
				
				// parsing request
				HTTPrequest request(&rawRequest);
				cout << request.getGet("request") << endl;

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
					closeConn(&recvSock);

				// unknown action
				} else {
					recvSock.send("unknown action");
					closeConn(&recvSock);
				}

				
				// any reading sockets left to be processed
				if (--ready <= 0) {
					break;
				}
			}
		}
		
	}

}

// handles requests from players
void Server::handlePlayerRequest(HTTPrequest* request, Socket* sock) {
	
	// TODO: direkt das player objekt zurückgeben und dann immer den funktionen übergeben. spart die suche im players container
	Game* myGame;
	int playerId = atoi(request->getGet("pId").c_str());
	int gameId = atoi(request->getGet("gId").c_str());
	// get Game object
	// check if there is a player with this id and authentication succeeded
	if (games.count(gameId) == 1) {
		myGame = games[gameId];
	} else {
		// game not found
		error(sock, "game not found");
		return;
	}
	
	string errorMsg;
	// authenticate player
	if (myGame->authenticate(playerId, request->getGet("authcode"))) {
		// responding to game request
		string gameRequest = request->getGet("request");
		
		// player wants to get actions
		if (gameRequest == "getActions") {
			
			// get since argument and check if not empty
			int since = atoi(request->getGet("since").c_str());
			
			// create request object
			PlayerRequest* newRequest = new PlayerRequest(playerId, since, sock->getSock());
			
			// send actions if there are already new actions
			if (!sendActions(myGame, newRequest)) {
				// put player into waiting list and send actions later when they occur
				myGame->requestsWaiting.push_back(newRequest);
				requestsWaitingSocks[sock->getSock()] = newRequest;
			// actions already sent -> destroy request
			} else {
				delete newRequest;
			}


		// player performed an action
		} else if (gameRequest == "registerAction") {
			// register action
			if (myGame->registerAction(playerId, request->getGet("action"), request->getGet("content"))) {
				// success
				HTTPresponse httpResponse;
				JSONobject jsonResponse;
				
				// prepare json response and add to body
				jsonResponse.addChild("result", "ok");
				httpResponse << "result = " + jsonResponse.str() + ";";
				
				// send response to player
				httpResponse.send(sock);
				
				// close and cleanup everything
				closeConn(sock);

				// send new actions to waiting players
				sendToWaiting(myGame);
				
			} else {
				errorMsg = "unknown error while registering your action";
			}


		// unknown request -> close
		} else {
			errorMsg = "unknown request";
		}

	// authentication failed -> close
	} else {
		errorMsg = "authentication failed";
	}
	
	// some error occurred ?
	if (!errorMsg.empty()) {
		error(sock, errorMsg);
	}
	
}

// send all actions to waiting players
void Server::sendToWaiting(Game* game) {
	// notifying all waiting players of the new actions
	vector<PlayerRequest*>::iterator rIter;
	for (rIter = game->requestsWaiting.begin(); rIter != game->requestsWaiting.end(); ++rIter) {
		// check if connection is not closed
		if ((*rIter)->sock > -1) {
			// connection still alive -> send new actions
			sendActions(game, *rIter);
		}
		// destroy request
		delete *rIter;
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
			} else if (request->getGet("request") == "start") {
				mIter->second->start();
				return true;
			}
		}
	}
	return false;
}

// send actions to players in waiting list
bool Server::sendActions(Game* myGame, PlayerRequest* request) {
	
	vector<Action*>::iterator aIter;
	vector<Action*> *newActions;
	pair<vector<Action*>, int> actions;
	
	// get all actions since since
	actions = myGame->getActionsSince(request->playerId, request->since);
	// no new actions ?
	if (actions.second-request->since < 1) {
		// stop here - request gets on waiting list
		return false;
	}
	
	// prepare response
	JSONobject jsonResponse;
	JSONarray* actionsArray = jsonResponse.addArray("actions");
	
	// go through all actions
	newActions = &(actions.first);
	for (aIter = newActions->begin(); aIter != newActions->end(); ++aIter) {
		JSONobject* action = actionsArray->addObject();
		action->addChild("action", (*aIter)->action);
		stringstream pId;
		pId << (*aIter)->player->getId();
		action->addChild("player", pId.str());
		action->addChild("content", (*aIter)->content);
	}
	
	// conclude response with the number of the last action
	stringstream lastAction;
	lastAction << actions.second;
	jsonResponse.addChild("lastAction", lastAction.str());
	
	// initiate http response
	HTTPresponse httpResponse;
	
	// prepare json response and add to body
	httpResponse << "game.registerActions(" + jsonResponse.str() + ");";
	
	// send response to player
	actionsSock.setSock(request->sock);
	httpResponse.send(&actionsSock);
	
	// close and cleanup everything
	closeConn(&actionsSock);
	
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

// send error back to client
void Server::error(Socket* sock, string msg) {
	// initiate http response
	HTTPresponse rawResponse;
	
	// prepare json response and add to body
	rawResponse << "an error occurred";
	if (!msg.empty()) {
		rawResponse << ": ";
		rawResponse << msg;
	}
	// send response to player
	rawResponse.send(sock);
	
	// close and cleanup everything
	closeConn(sock);
}

// closes a connection and removes a socket from set
void Server::closeConn(Socket* sock) {
	// close connection
	sock->close();
	// remove from set
	FD_CLR(sock->getSock(), &sockSet);
	// reset socket number
	connSocks[sock->getSock()] = -1;
}