#include <cstdlib>
using namespace std;
#include "socket.h"

// create a new socket with new file descriptor
void Socket::create() {
	sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if ( sock < 0 ) {
		throw SockExcept("error creating socket");
	}
	int y = 1;
	// set option the reuse address and port
	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int) );
}

// destructor
//Socket::~Socket() {
	// close socket
	//if (isValid()) ::close(sock);
//}

// binds socket to address and port
bool Socket::bind(const int port) {
	if (!isValid()) return false;
	
	// set socket address infos
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = INADDR_ANY;
	sockAddr.sin_port = htons(port);

	if ( ::bind( sock, (struct sockaddr*) &sockAddr, sizeof(sockAddr) ) < 0 ) return false;
	return true;
}

// socket must listen to address
bool Socket::listen() const {
	if (!isValid()) return false;
	
	if ( ::listen(sock, MAXCONNECTIONS < 0 ) ) return false;
	return true;
}

// accept connection
bool Socket::accept( Socket& newSocket ) const {
	int addrLen = sizeof(sockAddr);
	newSocket.sock = ::accept( sock, (sockaddr*) &sockAddr, (socklen_t*) &addrLen );
	if (newSocket.sock <= 0) return false;
	return true;
}

// create connection
bool Socket::connect( const string host, const int port ) {
	if (!isValid()) return false;
	struct hostent *hostInfo;
	unsigned long addr;
	
	memset( &sockAddr, 0, sizeof(sockAddr) );
	if ( (addr = inet_addr(host.c_str())) != INADDR_NONE ) {
		// numeric address
		memcpy( (char*) &sockAddr.sin_addr, &addr, sizeof(addr) );
	
	} else {
		// resolve hostname
		hostInfo = gethostbyname( host.c_str() );
		if (hostInfo == NULL) {
			throw SockExcept("error: unknown hostname");
		}
		memcpy( (char *) &sockAddr.sin_addr, hostInfo->h_addr, hostInfo->h_length);
	}
	
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(port);
	
	if ( ::connect(sock, (sockaddr *) &sockAddr, sizeof(sockAddr)) == 0 ) {
		return true;
	}
	return false;
}

// send data (TCP)
bool Socket::send(const string data) const {
	if ( ::send(sock, data.c_str(), data.size(), 0) ) {
		return true;
	}
	return false;
}

// receive data (TCP)
int Socket::recv(string& s, int max) const {
	// set data buffer - max contains the amount of data to be received
	char buf[max + 1];
	s = "";
	memset(buf, 0, sizeof(buf));
	int status = ::recv(sock, buf, max, 0);
	if (status > 0 || status != -1) {
		s = buf;
		return status;
	} else {
		//throw SockExcept("error receiving data");
		return 0;
	}

}

// close sockets
bool Socket::close() const {
	::close(sock);
	return true;
}