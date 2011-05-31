#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
using namespace std;

#include "webapi.h"
#include "httprequest.h"

// host of server
string WebAPI::serverAddr = "7st.a0s.de";
string WebAPI::authcode = "";
// define other static symbols
boost::asio::io_service* WebAPI::ioService = NULL;
boost::asio::ip::tcp::resolver::iterator WebAPI::rIter;

void WebAPI::makeRequest(JSONobject* jsonRequest, string action, boost::function<void()> finishHandler) {
	HTTPrequestPtr httpRequest = prepareHTTPrequest(jsonRequest, action);
	
	// create new request and send it but handle response later
	boost::shared_ptr<WebAPIrequest> request = createRequest(httpRequest, finishHandler);
	request->send();
}

JSONoPtr WebAPI::makeSyncRequest(JSONobject* jsonRequest, string action) {
	HTTPrequestPtr httpRequest = prepareHTTPrequest(jsonRequest, action);
	
	// create new request, send it and process response now
	boost::shared_ptr<WebAPIrequest> request = createRequest(httpRequest, boost::function<void()>());
	HTTPresponsePtr httpResponse = request->sendSync();
	
	// parse json object
	string body;
	httpResponse->getBody(&body);
	return JSONoPtr(new JSONobject(body));
}

HTTPrequestPtr WebAPI::prepareHTTPrequest(JSONobject* jsonRequest, string action) {
	// add authcode and request
	jsonRequest->addChild("request", action);
	jsonRequest->addChild("authcode", authcode);
	
	// prepare http request
	HTTPrequestPtr httpRequest(new HTTPrequest(serverAddr, "/butler.php"));
	httpRequest->setPost("request", jsonRequest->str());

	return httpRequest;
}

// creates new request object and adds it to the set
boost::shared_ptr<WebAPIrequest> WebAPI::createRequest(HTTPrequestPtr httpRequest, boost::function<void()> finishHandler) {
	boost::shared_ptr<WebAPIrequest> request(new WebAPIrequest(ioService, rIter, httpRequest, this, finishHandler));
	requests.insert(request);
	return request;
}

void WebAPI::removeRequest(requestPtr request) {
	requests.erase(request);
}

void WebAPIrequest::send() {
	socket.async_connect(*rIter, boost::bind(&WebAPIrequest::handleConnect, this, boost::asio::placeholders::error));
}

void WebAPIrequest::handleConnect(const boost::system::error_code& error) {
	if (error) return;
	boost::asio::async_write(socket, boost::asio::buffer(httpRequest->getAll()), boost::bind(&WebAPIrequest::handleWrite, this, boost::asio::placeholders::error));
}

void WebAPIrequest::handleWrite(const boost::system::error_code& error) {
	close(error);
}

void WebAPIrequest::handleRead(const boost::system::error_code& error) {
	close(error);
}

void WebAPIrequest::close(const boost::system::error_code& error) {
	if (!error) {
		//socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
	}
	socket.close();
	
	// some handler to call after finishing this request ?
	if (finishHandler) {
		finishHandler();
	
	// only remove request if no handler is specified - request gets removed either way if object is destroyed
	} else {
		// remove request
		wAPI->removeRequest(shared_from_this());
	}
}

// send a request synchronously to get an immediate response
HTTPresponsePtr WebAPIrequest::sendSync() {
	stringstream rawResponse;
	boost::system::error_code error;
	
	// connect and send request
	socket.connect(*rIter);
	boost::asio::write(socket, boost::asio::buffer(httpRequest->getAll()));

	// get response
	boost::asio::read(socket, buffer, boost::asio::transfer_at_least(1), error);
	rawResponse << &buffer;
	HTTPresponsePtr response(new HTTPresponse(rawResponse));
	
	// copy body
	string line;
	while (getline(rawResponse, line)) {
		line += "\n";
		*response << line;
	}

	close(error);
	return response;
}