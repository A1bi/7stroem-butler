#include "server.h"


// starts game server
void Server::start() {
	// register this butler with web server on startup
	if (!registerWithServer()) {
		cout << "could not register with webserver" << endl;
		exit(1);
	}
    
	listen();
	ioService.run();
}

// listen for new connections, accept and pass them on to a new thread of handleNewConnection()
void Server::listen() {
	acceptor.async_accept(newConn->getSock(), boost::bind(&Server::handleAccept, this));
}

// open new socket, accept new connections and let it be handle by handleNewConnection()
void Server::handleAccept() {
	newConn->read();
	connections.insert(newConn);
	newConn.reset(new Connection(ioService, this));
	acceptor.async_accept(newConn->getSock(), boost::bind(&Server::handleAccept, this));
}

// checks for missing players and removes them from their games if neccessary
void Server::removeGame(int id) {
	games.erase(id);
}

// handles an incoming connection after being accepted by the listening socket
void Server::handleNewRequest(connectionPtr conn) {	
	HTTPrequestPtr request = conn->getHttpRequest();
	
	// it's a player request
	if (request->getUri() == "/player") {
		handlePlayerRequest(conn);
	
	// it's something else
	} else {
		string response;
		// request by the 7stroem server
		if (request->getUri() == "/server") {
			if (request->getGet("authcode") == authcode && handleServerRequest(conn)) {
				response = "ok";
			} else {
				response = "error";
			}
		// user only wants to test the connection
		} else if (request->getUri() == "/test") {
			response = "ok";
		// unknown action
		} else {
			response = "unknown action";
		}
		
		conn->respond(response);
	}
	
}

// handles requests from players
void Server::handlePlayerRequest(connectionPtr conn) {
	
	GamePtr myGame;
	HTTPrequestPtr request = conn->getHttpRequest();
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
		
		// authenticate player and get its object
		PlayerPtr tPlayer = myGame->authenticate(playerId, request->getGet("authcode"));
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
				PlayerRequestPtr newRequest(new PlayerRequest(myGame, tPlayer));
				// add to connection
				conn->setPlayerRequest(newRequest);
				
				// send actions if there are already new actions
				if (!sendActions(conn, since)) {
					// put player into waiting list and send actions later when they occur
					myGame->requestsWaiting.insert(conn);
				}				
				
			// player performed an action
			} else if (gameRequest == "registerAction" || gameRequest == "registerHostAction") {
				// success
				JSONoPtr jsonResponse = getOkResponse();
				
				if (gameRequest == "registerAction") {
					// register action
					myGame->registerAction(tPlayer, request->getGet("action"), request->getGet("content"));
					
					// if player flipped his hand we now have to send him his cards so he can see them
					if (request->getGet("action") == "flipHand") {
						JSONarray* jsonCards = jsonResponse->addArray("cards");
						
						string cards[4];
						tPlayer->getHand(cards);
						// no cards got ? -> player is out
						if (cards[0].empty()) {
							throw "you don't have any cards because you are out";
						}
						for (int i = 0; i < 4; i++) {
							jsonCards->addChild(cards[i]);
						}
					} else {
						// send new actions to waiting players
						sendToWaiting(myGame);
					}
				
				} else if (gameRequest == "registerHostAction") {
					myGame->registerHostAction(tPlayer, request->getGet("action"));
					sendToWaiting(myGame);
				}
				
				// send response
				conn->respond(jsonResponse);
				
				
			// player quit
			} else if (gameRequest == "quit") {
				tPlayer->quit();
				conn->respond(getOkResponse());
			
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
	catch (const char* msg) {
		sendError(conn, string(msg));
	}
	// action not allowed ?
	catch (ActionExcept& e) {
		sendError(conn, e.getErrorMsg(), e.getErrorId());
	}
	
}

// send all actions to waiting players
void Server::sendToWaiting(GamePtr game) {
	// notifying all waiting players of the new actions
	connectionSet::iterator rIter;
	for (rIter = game->requestsWaiting.begin(); rIter != game->requestsWaiting.end(); ++rIter) {
		// send new actions
		sendActions(*rIter);
	}
	
	game->finishNotification();
}

// handles requests from the remote 7stroem server
bool Server::handleServerRequest(connectionPtr conn) {
	HTTPrequestPtr request = conn->getHttpRequest();
	
	// create game
	if (request->getGet("request") == "createGame") {
		if (request->getGet("id") != "" && request->getGet("host") != "" && createGame(atoi(request->getGet("id").c_str()), atoi(request->getGet("host").c_str()))) {
			return true;
		}
	// requests with game id
	} else if (request->getGet("gId") != "") {
		// get game
		mGame::iterator mIter = games.find(atoi(request->getGet("gId").c_str()));
		if (mIter == games.end()) {
			// game not found
			return false;
		}
		GamePtr game = mIter->second;
		// register player
		if (request->getGet("request") == "registerPlayer") {
			if (request->getGet("pId") != "" && request->getGet("pAuthcode") != "") {
				if (!game->addPlayer( atoi(request->getGet("pId").c_str()), request->getGet("pAuthcode") )) {
					return false;
				}
				sendToWaiting(game);
				return true;
			}
		}
	}
	return false;
}

// send actions to players in waiting list
bool Server::sendActions(connectionPtr conn, int start) {
	PlayerRequestPtr request = conn->getPlayerRequest();
	
	pair<vector<Action*>, int> actions;
	actions.second = start;
	
	// get all actions since given event number
	request->game->getActionsSince(&actions);
	// no new actions ?
	if (actions.first.size() < 1) {
		// stop here - request gets on waiting list
		return false;
	}
	
	// prepare response
	JSONoPtr jsonResponse = getOkResponse();
	JSONarray* actionsArray = jsonResponse->addArray("actions");
	
	// go through all actions
	for (vector<Action*>::iterator aIter = actions.first.begin(); aIter != actions.first.end(); ++aIter) {
		JSONobject* action = actionsArray->addObject();
		action->addChild("action", (*aIter)->action);
		action->addChild("player", (*aIter)->player);
		action->addChild("content", (*aIter)->content);
	}
	
	// conclude response with the number of the last action
	jsonResponse->addChild("lastAction", actions.second);
	
	// send response to player
	conn->respond(jsonResponse);
	
	return true;
	
}

// creates a new game and stores it under given id
bool Server::createGame(int gameId, int host) {
	// check if game id not yet created
	if (games.find(gameId) == games.end()) {
		games[gameId] = GamePtr(new Game(gameId, host, this));
		return true;
	}
	return false;
}

// register this server with the webserver and retrieve the authcode the webserver will later use to authenticate with this server
bool Server::registerWithServer() {
	JSONobject json;
	JSONoPtr response = wAPI.makeSyncRequest(&json, "registerServer");
	if (response->getChild("result") == "ok") {
		authcode = response->getChild("authcode");
		return true;
	}
	return false;
}

// just send a response saying that the request was successfully executed
JSONoPtr Server::getOkResponse() {
	// create json response
	JSONoPtr jsonResponse(new JSONobject);
	// add okay child
	jsonResponse->addChild("result", "ok");
	
	return jsonResponse;
}

// send error back to player
void Server::sendError(connectionPtr conn, string msg, string id) {
	
	// success
	JSONoPtr jsonResponse(new JSONobject);
	// prepare json response and add to body
	jsonResponse->addChild("result", "error");
	JSONobject* error = jsonResponse->addObject("error");
	error->addChild("id", id);
	error->addChild("message", msg);
	
	// send to player
	conn->respond(jsonResponse);
}

// remove connection from list
void Server::removeConn(connectionPtr conn) {
	// get player request if there is one
	PlayerRequestPtr request = conn->getPlayerRequest();
	if (request) {
		// remove player request from waiting list
		request->game->requestsWaiting.erase(conn);
		request->player->setDisconnected();
	}

	// remove from connections list
	connections.erase(conn);
}