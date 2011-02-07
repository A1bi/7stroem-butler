#include <iostream>
using namespace std;
#include "server.h"


int main (int argc, char * const argv[]) {
	
	// example
	Server myServer;
	myServer.createGame(1);
	myServer.start();
	
    return 0;
}