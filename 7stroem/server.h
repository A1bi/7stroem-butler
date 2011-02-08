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

struct PlayerRequest {
	int playerId, since, sock;
	PlayerRequest(int p, int si, int so): playerId(p), since(si), sock(so) {}
};

class Server {
public:
	void start();
	void createGame(int);
	
private:
	int connSocks[FD_SETSIZE];
	fd_set sockSet, sockReadSet;
	Socket actionsSock;
	map<int, Game*> games;
	map<Game*, vector<PlayerRequest*> > requestsWaiting;
	map<int, PlayerRequest*> requestsWaitingSocks;
	void handlePlayerRequest(HTTPrequest*, Socket*);
	void closeConn(Socket*);
	bool sendActions(Game*, PlayerRequest*);
	void error(Socket*, string = "");
};

#endif