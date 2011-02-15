// server.h
#ifndef _SERVER_H_
#define _SERVER_H_
#include <string>
#include <vector>
#include <map>
#include "socket.h"
#include "game.h"
#include "httprequest.h"
using namespace std;

class Server {
public:
	void start();
	
private:
	int connSocks[FD_SETSIZE];
	fd_set sockSet, sockReadSet;
	Socket actionsSock;
	map<int, Game*> games;
	map<int, PlayerRequest*> requestsWaitingSocks;
	void handlePlayerRequest(HTTPrequest*, Socket*);
	bool handleServerRequest(HTTPrequest*);
	void closeConn(Socket*);
	bool sendActions(Game*, PlayerRequest*);
	void sendToWaiting(Game*);
	void error(Socket*, string = "");
	bool createGame(int);
	static const string authcode;
};

#endif