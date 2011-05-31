#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
using namespace std;

#include "types.h"
#include "httprequest.h"
#include "json.h"

class Server;

class Connection : public boost::enable_shared_from_this<Connection> {
	public:
	Connection(boost::asio::io_service& io_service, Server* server) :
		socket(io_service),
		gameServer(server),
		timerTimeout(io_service),
		closed(false) {}
	boost::asio::ip::tcp::socket& getSock() {
		return socket;
	}
	~Connection() {
		timerTimeout.cancel();
	}
	void read();
	void handleRead(const boost::system::error_code&, std::size_t);
	void handleClose(const boost::system::error_code&);
	void close();
	HTTPrequestPtr getHttpRequest() {
		return httpRequest;
	}
	PlayerRequestPtr getPlayerRequest() {
		return playerRequest;
	}
	void setPlayerRequest(PlayerRequestPtr request) {
		playerRequest = request;
	}
	void respond(string);
	void respond(JSONoPtr);
	void handleWrite();
	void timeout(const boost::system::error_code&);
	
	private:
	boost::asio::ip::tcp::socket socket;
	boost::asio::streambuf readData;
	boost::asio::deadline_timer timerTimeout;
	Server* gameServer;
	HTTPrequestPtr httpRequest;
	PlayerRequestPtr playerRequest;
	bool closed;

};

#endif