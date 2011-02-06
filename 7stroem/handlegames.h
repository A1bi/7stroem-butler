// handlegames.h
#include <iostream>
using namespace std;
#ifndef _HANDLEGAMES_H_
#define _HANDLEGAMES_H_

class HandleGames {
	public:
		bool create(int &players);
		Game get(int &id);

	private:
		Game* games[100];
};


bool HandleGames::create(int &players) {

}

Game HandleGames::get(int &id) {
	return 1;
}

#endif