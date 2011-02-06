// socket.h
#ifndef _SOCKET_H_
#define _SOCKET_H_
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

// maximum number of concurrent connections
const int MAXCONNECTIONS = 5;
// maximum amount of data to be received
const int MAXRECV = 1024;


class Socket {
	
public:
	// constructors
	Socket();
	virtual ~Socket();
	
	bool bind(const int port);
	bool listen() const;
	bool accept(Socket&) const;
	bool connect(const string host, const int port);
	bool send(const string) const;
	int recv(string&, int = MAXRECV) const;
	bool close() const;
	// for windows
	void cleanup() const {}
	bool isValid() const {
		return sock != -1;
	}
	
private:
	// socket descriptor
	int sock;
	sockaddr_in sockAddr;
	
};


// socket exceptions
class SockExcept {
public:
	SockExcept(string s): except(s) {}
	string getSockExcept() {
		return except;
	}

private:
	string except;
	
};

#endif