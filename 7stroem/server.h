// server.h
#ifndef _SERVER_H_
#define _SERVER_H_

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
using namespace std;

#include "types.h"
#include "game.h"
#include "httprequest.h"
#include "json.h"
#include "webapi.h"
#include "connection.h"

typedef map<int, GamePtr> mGame;

struct PlayerRequest : public boost::enable_shared_from_this<PlayerRequest> {
	GamePtr const game;
	PlayerPtr const player;
	PlayerRequest(GamePtr g, PlayerPtr p): game(g), player(p) {}
};

class Server {
public:
	Server(const int port) :
		authcode("zGLqz2QM5RGQkwld"), acceptor(ioService,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
		newConn(new Connection(ioService, this)) {
			WebAPI::setIOservice(&ioService);
	}
	void start();
	void handleNewRequest(connectionPtr);
	void removeConn(connectionPtr);
	void removeGame(int);
	void sendToWaiting(GamePtr gameCon);
	boost::asio::io_service* getIO() {
		return &ioService;
	}
	
private:
	
	mGame games;
	string authcode;
	WebAPI wAPI;
	connectionSet connections;
    boost::asio::io_service ioService;
	boost::asio::ip::tcp::acceptor acceptor;
	connectionPtr newConn;
    
	void listen();
    void handleAccept();
	void handlePlayerRequest(connectionPtr);
	bool handleServerRequest(connectionPtr);
	bool sendActions(connectionPtr, int = -1);
	void sendError(connectionPtr, string, string = "");
	bool createGame(int, int);
	bool registerWithServer();
	JSONoPtr getOkResponse();
	
};

#endif