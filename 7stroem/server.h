// server.h
#ifndef _SERVER_H_
#define _SERVER_H_
#include <string>
#include <vector>
#include <map>
#include "boost/thread/thread.hpp"
#include "socket.h"
#include "game.h"
#include "httprequest.h"
#include "json.h"
using namespace std;

class Server {
public:
	void start();
	
private:
	map<int, Game*> games;
	vector<PlayerRequest*> openConnections;
	vector<Player*> missingPlayers;
	static const string authcode;
	
	void checkConnections();
	void checkMissingPlayers();
	void listen(Socket*);
	void handleNewConnection(int);
	void handlePlayerRequest(HTTPrequest*, Socket*);
	bool handleServerRequest(HTTPrequest*);
	bool sendActions(PlayerRequest*);
	void sendToWaiting(Game*);
	void sendResponse(Socket* sock, JSONobject* response);
	void sendError(Socket*, string = "", string = "");
	bool createGame(int);
	
	// mutexes
	boost::mutex mutexConn;
	
};

#endif