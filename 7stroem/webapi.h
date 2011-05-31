// webapi.h
#ifndef _WEBAPI_H_
#define _WEBAPI_H_

#include <string>
#include <set>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
using namespace std;

#include "types.h"
#include "json.h"
#include "httprequest.h"

class WebAPI;

class WebAPIrequest : public boost::enable_shared_from_this<WebAPIrequest> {
	public:
	WebAPIrequest(boost::asio::io_service* io, boost::asio::ip::tcp::resolver::iterator r, HTTPrequestPtr h, WebAPI* w, boost::function<void()> f) :
		socket(*io),
		rIter(r),
		httpRequest(h),
		wAPI(w),
		finishHandler(f) {};
	void send();
	HTTPresponsePtr sendSync();
	
	private:
	boost::asio::ip::tcp::socket socket;
	boost::asio::ip::tcp::resolver::iterator rIter;
	boost::asio::streambuf buffer;
	WebAPI* wAPI;
	HTTPrequestPtr httpRequest;
	boost::function<void()> finishHandler;
	
	void handleConnect(const boost::system::error_code&);
	void handleWrite(const boost::system::error_code&);
	void handleRead(const boost::system::error_code&);
	void close(const boost::system::error_code&);
	
};

typedef boost::shared_ptr<WebAPIrequest> requestPtr;

class WebAPI {
	public:
	static void setServerAddr(string b) {
		serverAddr = b;
	}
	static void setAuthcode(string b) {
		authcode = b;
	}
	static void setIOservice(boost::asio::io_service* io) {
		// set IO
		ioService = io;
		
		// resolve hostname
		boost::asio::ip::tcp::resolver::query query(serverAddr, "80");
		boost::asio::ip::tcp::resolver resolver(*ioService);
		rIter = resolver.resolve(query);
	}
	void makeRequest(JSONobject*, string, boost::function<void()>);
	JSONoPtr makeSyncRequest(JSONobject*, string);
	void removeRequest(requestPtr);
	
	private:
	static string serverAddr, authcode;
	static boost::asio::io_service* ioService;
	static boost::asio::ip::tcp::resolver::iterator rIter;
	set<requestPtr> requests;
	
	requestPtr createRequest(HTTPrequestPtr, boost::function<void()>);
	HTTPrequestPtr prepareHTTPrequest(JSONobject*, string);
	
};

#endif