#include <boost/bind.hpp>

#include "connection.h"
#include "server.h"

void Connection::read() {
	boost::asio::async_read_until(socket, readData, "\r\n\r\n", boost::bind(&Connection::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Connection::handleRead(const boost::system::error_code& error, std::size_t bytes) {
	if (!error) {
		// parse http request
		stringstream data;
		data << &readData;
		httpRequest = HTTPrequestPtr (new HTTPrequest(data));
		
		// callback server to handle this request
		gameServer->handleNewRequest(shared_from_this());
		
		// start timeout timer
		timerTimeout.expires_from_now(boost::posix_time::seconds(45));
		timerTimeout.async_wait(boost::bind(&Connection::timeout, this, boost::asio::placeholders::error));
		
		// start async_read again to detect closed connections
		boost::asio::async_read(socket, readData, boost::bind(&Connection::handleClose, this, boost::asio::placeholders::error));
	
	// connection closed
	} else {
		close(false);
	}
}

// is called after the request was already read -> must be a closed connection now
void Connection::handleClose(const boost::system::error_code& error) {
	if (error != boost::asio::error::operation_aborted) {
		cout << "connection closed" << endl;
		close(false);
	}
}

// shuts down the socket and closes the connection
void Connection::close(const bool shutdown) {
	if (closed) return;
	closed = true;
	
	if (shutdown) {
		//socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	}
	
	socket.close();
	// let the server remove this connection from list
	gameServer->removeConn(shared_from_this());
}

// respond with a JSON object
void Connection::respond(boost::shared_ptr<JSONobject> response) {
	respond(response->str());
}

// respond with a simple text message
void Connection::respond(string msg) {
	// generate http response
	HTTPresponse response;
	response << msg;
	
	// send response
	boost::asio::async_write(socket, boost::asio::buffer(response.getAll()), boost::bind(&Connection::handleWrite, this));
}

// function to call after response was sent to the client
void Connection::handleWrite() {
	close(true);
}

// function is called by a timer
// sends empty response
void Connection::timeout(const boost::system::error_code& error) {
	if (error) return;
	
	// prepare a json with just a "timeout" as a result
	// "timeout" tells the client to reconnect
	cout << "timeout" << endl;
	boost::shared_ptr<JSONobject> response(new JSONobject);
	response->addChild("result", "timeout");
	
	respond(response);
}