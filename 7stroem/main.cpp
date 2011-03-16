#include <iostream>
using namespace std;
#include "server.h"


int main (int argc, char * const argv[]) {
	
	// initiate server
	Server myServer;
	myServer.start();
	
    return 0;
}