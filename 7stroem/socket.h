// socket.h
#ifndef _SOCKET_H_
#define _SOCKET_H_
#include <cstring>
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
	Socket() : sock(0) {}
	//virtual ~Socket();
	
	void create();
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
		return (sock != -1);
	}
	// returns socket number
	int getSock() {
		return sock;
	}
	// sets new socket number
	void setSock(int newSock) {
		sock = newSock;
	}
	
private:
	// socket descriptor and number
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