#include <iostream>
using namespace std;

#include "server.h"
#include "webapi.h"


int main (int argc, char * const argv[]) {
	
	bool err = false;
	int opt, port = 4926;
	extern char *optarg;
	extern int optopt;
	
	while ((opt = getopt(argc, argv, ":p:w:a:")) != -1) {
		switch (opt) {
				
			case 'p':
				port = atoi(optarg);
				break;
			case 'w':
				WebAPI::setServerAddr(optarg);
				break;
			case 'a':
				WebAPI::setAuthcode(optarg);
				break;
				
			case ':':
				cout << "option -" << char(optopt) << " requires a value\n";
				err = true;
				break;
			case '?':
				cout << "unknown option -" << char(optopt) << "\n";
				err = true;
				break;
				
		}
	}
	if (err) {
		cout << "usage: 7stroem [-p bind_port] [-w WebAPI_address] [-a WebAPI_authcode]\n";
		exit(2);
    }
	
	// initiate server
	Server server(port);
	server.start();
	
    return 0;
}