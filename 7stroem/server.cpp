#include "server.h"

#include <iostream>

const string Server::authcode = "zGLqz2QM5RGQkwld";

// starts game server
void Server::start() {
	
	// create listening sock whick checks for new connections
	Socket listeningSock;
	listeningSock.create();
	listeningSock.bind(4926);
	listeningSock.listen();
	// create new connection checking thread
	boost::thread listening(boost::bind(&Server::listen, this, &listeningSock));
	
	// check connections and missing players periodically
	boost::thread checkingConns(boost::bind(&Server::checkConnections, this));
	boost::thread checkingPlayers(boost::bind(&Server::checkPlayers, this));
	
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
	vPRequest::iterator vIter;
	fd_set sockSet;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500;
	
	while (true) {
		boost::this_thread::sleep(boost::posix_time::seconds(2));
		
		FD_ZERO(&sockSet);
		sockMax = -1;
		boost::lock_guard<boost::mutex> lock(mutexConn);
		// create a socket set which contains all open connections
		for (vIter = openConnections.begin(); vIter != openConnections.end(); ++vIter) {
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
					// remove from set
					FD_CLR((*vIter)->sock, &sockSet);
					// remove from request list
					{
						boost::lock_guard<boost::mutex> lock(mutexGames);
						(*vIter)->gameCon->requestsWaiting.erase(find((*vIter)->gameCon->requestsWaiting.begin(), (*vIter)->gameCon->requestsWaiting.end(), *vIter));
					}
					delete *vIter;
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
void Server::checkPlayers() {
	mGameCon::iterator mIter;
	
	while (true) {
		boost::this_thread::sleep(boost::posix_time::seconds(3));

		boost::lock_guard<boost::mutex> lock(mutexGames);
		for (mIter = games.begin(); mIter != games.end(); ) {
			GameContainer* gameCon = mIter->second;
			Game* game = gameCon->game;
			
			// lock mutex
			//boost::mutex::scoped_lock gameLock(gameCon->mutex);
			
			// check if any players have left the game
			int action = game->checkPlayers();
			if (action == 2) {
				// no players left -> remove game
				games.erase(mIter++);
				
				// destroy object
				delete gameCon;
			} else {
				if (action == 1) {
					sendToWaiting(gameCon);
				}
				mIter++;
			}
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
	
	// it's something else
	} else {
		HTTPresponse response;
		// request by the 7stroem server
		if (request.getUri() == "/server") {
			if (request.getGet("authcode") == authcode && handleServerRequest(&request)) {
				response << "ok";
			} else {
				response << "error";
			}
		// user only wants to test the connection
		} else if (request.getUri() == "/test") {
			response << "ok";
		// unknown action
		} else {
			response << "unknown action";
		}
		response.send(&recvSock);
		recvSock.close();
	}
	
}

// handles requests from players
void Server::handlePlayerRequest(HTTPrequest* request, Socket* sock) {
	
	GameContainer* myGameCon;
	Game* myGame;
	int playerId = atoi(request->getGet("pId").c_str());
	int gameId = atoi(request->getGet("gId").c_str());
	bool jsonp = (request->getGet("jsonp") == "") ? false : true;
	
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
		
		// lock games
		boost::lock_guard<boost::mutex> lock(mutexGames);
		
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
				PlayerRequest* newRequest = new PlayerRequest(myGameCon, tPlayer, since, sock->getSock(), jsonp);
				
				// send actions if there are already new actions
				if (!sendActions(newRequest)) {
					boost::lock_guard<boost::mutex> lock(mutexConn);
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
						// no cards got ? -> player is out
						if (cards[0].empty()) {
							throw "you don't have any cards because you are out";
						}
						for (int i = 0; i < 4; i++) {
							jsonCards->addChild(cards[i]);
						}
					} else {
						// send new actions to waiting players
						sendToWaiting(myGameCon);
					}
				
				} else if (gameRequest == "registerHostAction") {
					myGame->registerHostAction(tPlayer, request->getGet("action"));
					sendToWaiting(myGameCon);
				}
				
				// send response
				sendResponse(sock, &jsonResponse, jsonp);
				
				
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
		sendError(sock, jsonp, string(msg));
	}
	// action not allowed ?
	catch (ActionExcept& e) {
		sendError(sock, jsonp, e.getErrorMsg(), e.getErrorId());
	}
	// safety net: some other error occurred, games has crashed -> bug
	catch (...) {
		sock->close();
		// remove game
		games.erase(gameId);
		// try to notify all players that game has crashed
		try {
			Socket sock;
			boost::lock_guard<boost::mutex> lock(mutexConn);
			for (vPRequest::iterator rIter = myGameCon->requestsWaiting.begin(); rIter != myGameCon->requestsWaiting.end(); ++rIter) {
				sock.setSock((*rIter)->sock);
				sendError(&sock, (*rIter)->jsonp, "the game crashed. seems to be a bug. sorry!", "fatalCrash");
			}
		}
		// couldn't even do this. bummer.
		catch (...) {
			
		}
		// destroy object
		delete myGameCon;
	}
	
}

// send all actions to waiting players
void Server::sendToWaiting(GameContainer* gameCon) {
	// notifying all waiting players of the new actions
	vPRequest::iterator rIter;
	// make sure it doesn't interfere with checkConnections()
	boost::lock_guard<boost::mutex> lock(mutexConn);
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
		mGameCon::iterator mIter = games.find(atoi(request->getGet("gId").c_str()));
		if (mIter == games.end()) {
			// game not found
			return false;
		}
		GameContainer* gameCon = mIter->second;
		// register player
		if (request->getGet("request") == "registerPlayer") {
			if (request->getGet("pId") != "" && request->getGet("pAuthcode") != "") {
				Player* newPlayer = gameCon->game->addPlayer( atoi(request->getGet("pId").c_str()), request->getGet("pAuthcode") );
				if (newPlayer == NULL) {
					return false;
				}
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
	sendResponse(&actionsSock, &jsonResponse, request->jsonp);
	
	// mark this player's client as disconnected
	request->player->setDisconnected();
	
	return true;
	
}

// creates a new game and stores it under given id
bool Server::createGame(int gameId, int host) {
	// check if game id not yet created
	boost::lock_guard<boost::mutex> lock(mutexGames);
	if (games.find(gameId) == games.end()) {
		games[gameId] = new GameContainer(gameId, host);
		return true;
	}
	return false;
}

// send a response as a JSON object to player
void Server::sendResponse(Socket* sock, JSONobject* response, bool jsonp) {
	// prepare reponse
	HTTPresponse httpResponse;
	if (jsonp) {
		httpResponse << "game.processResponse(" + response->str() + ");";
	} else {
		httpResponse << response->str();
	}

	// send response to player
	httpResponse.send(sock);
	
	// close and cleanup everything
	sock->close();
}

// send error back to player
void Server::sendError(Socket* sock, bool jsonp, string msg, string id) {
	
	// success
	JSONobject jsonResponse;
	// prepare json response and add to body
	jsonResponse.addChild("result", "error");
	JSONobject* error = jsonResponse.addObject("error");
	error->addChild("id", id);
	error->addChild("message", msg);
	
	// send to player
	sendResponse(sock, &jsonResponse, jsonp);
}