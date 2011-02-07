#include "server.h"
#include "json.h"
#include <iostream>

// constructor
Server::Server() {
	max = -1;
}

// initiates game server
void Server::start() {
	
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
				throw SockExcept("too many clients. haltung!");
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
		// TODO: check if remote client closed connection
		for (i = 0; i <= max; i++) {
			string rawRequest;
			
			recvSock.setSock(connSocks[i]);
			if ((recvSock.getSock()) < 0) {
				continue;
			}
			
			// socket in reading socket set -> new data to read (receive) ?
			if (FD_ISSET(recvSock.getSock(), &sockReadSet)) {
				// receive data - getting request
				recvSock.recv(rawRequest);
				
				// parsing request
				HTTPrequest request(&rawRequest);
				
				// it's a player request
				if (request.getUri() == "/player") {
					handlePlayerRequest(&request, &recvSock);
				}
				
				// any reading sockets left to be processed
				if (--ready <= 0) {
					break;
				}
			}
		}
		
	}

}

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
				requestsWaiting[myGame].push_back(newRequest);
			}

						
		// unknown request -> close
		} else {
			error(sock, "unknown request");
		}

	// authentication failed -> close
	} else {
		error(sock, "authentication failed");
	}

	
}

bool Server::sendActions(Game* myGame, PlayerRequest* request) {
	
	vector<Action*>::iterator aIter;
	vector<Action*> *newActions;
	pair<vector<Action*>, int> actions;
	
	// get all actions since since
	actions = myGame->getActionsSince(request->playerId, request->since);cout << actions.second << endl;
	// no new actions ?
	if (actions.second-request->since < 1) {
		// stop here - request gets on waiting list
		return false;
	}
	
	// prepare response
	JSONobject response;
	JSONarray* actionsArray = response.addArray("actions");
	
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
	response.addChild("lastAction", lastAction.str());
	
	// initiate http response
	HTTPresponse rawResponse;
	
	// prepare json response and add to body
	rawResponse << "blubb = ";
	rawResponse << response.str();
	rawResponse << ";";
	
	// send response to player
	actionsSock.setSock(request->sock);
	rawResponse.send(&actionsSock);
	
	// close and cleanup everything
	closeConn(&actionsSock);
	// delete request object
	delete request;
	
	return true;
	
}

// creates a new game and stores it under given id
void Server::createGame(int gameId) {
	Game* newGame = new Game;
	// test values !!!!
	newGame->addPlayer(123, "bla");
	newGame->addPlayer(124, "bla");
	newGame->addPlayer(125, "bla");
	newGame->startGame();
	games[gameId] = newGame;
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
	sock->close();
	FD_CLR(sock->getSock(), &sockSet);
	connSocks[i] = -1;
}