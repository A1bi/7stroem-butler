#ifndef _TYPES_H_
#define _TYPES_H_

#include <boost/shared_ptr.hpp>
#include <set>

struct Connection;
struct PlayerRequest;
struct Game;
struct Player;
struct HTTPresponse;
struct HTTPrequest;
struct JSONobject;

// shared pointers
typedef boost::shared_ptr<Connection> connectionPtr;
typedef boost::shared_ptr<PlayerRequest> PlayerRequestPtr;
typedef boost::shared_ptr<Game> GamePtr;
typedef boost::shared_ptr<Player> PlayerPtr;
typedef boost::shared_ptr<HTTPresponse> HTTPresponsePtr;
typedef boost::shared_ptr<HTTPrequest> HTTPrequestPtr;
typedef boost::shared_ptr<JSONobject> JSONoPtr;

// containers
typedef set<connectionPtr> connectionSet;

#endif