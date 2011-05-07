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
#include "webapi.h"
using namespace std;

struct GameContainer;

struct PlayerRequest {
	const int since, sock;
	bool jsonp;
	GameContainer* const gameCon;
	Player* const player;
	PlayerRequest(GameContainer* g, Player* p, int si, int so, bool j): gameCon(g), player(p), since(si), sock(so), jsonp(j) {}
};

struct GameContainer {
	Game* const game;
	vector<PlayerRequest*> requestsWaiting;
	//boost::mutex mutex;
	
	GameContainer(int id, int host): game(new Game(id, host)) {}
	~GameContainer() {
		delete game;
	}
	
};

class Server {
public:
	Server() : authcode("zGLqz2QM5RGQkwld") {}
	void start(const int);
	
private:
	typedef map<int, GameContainer*> mGameCon;
	typedef vector<PlayerRequest*> vPRequest;
	mGameCon games;
	vPRequest openConnections;
	string authcode;
	WebAPI wAPI;
	
	void checkConnections();
	void checkPlayers();
	void listen(Socket*);
	void handleNewConnection(int);
	void handlePlayerRequest(HTTPrequest*, Socket*);
	bool handleServerRequest(HTTPrequest*);
	bool sendActions(PlayerRequest*);
	void sendToWaiting(GameContainer* gameCon);
	void sendResponse(Socket* sock, JSONobject* response, bool);
	void sendError(Socket*, bool, string, string = "");
	bool createGame(int, int);
	bool registerWithServer();
	
	// mutexes
	boost::mutex mutexConn, mutexGames;
	
};

#endif