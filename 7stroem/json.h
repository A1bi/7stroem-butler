// json.h
#ifndef _JSON_H_
#define _JSON_H_
#include <string>
#include <vector>
#include "boost/regex.hpp"
using namespace std;

// TODO: TEMPLAAAAATES du arsch
// JSON generating stuff

class JSONchild;
class JSONarray;

class JSON {
protected:
	string getTabs(int);
};

class JSONobject : public JSON {
public:
	JSONobject() {};
	JSONobject(string);
	~JSONobject();
	void addChild(string, string);
	JSONarray* addArray(string, string = "");
	JSONobject* addObject(string, string = "");
	string str(int = 0);
	string getChild(string);
	JSONarray* getArray(string);
	
protected:
	void pushChild(string, JSONchild*);
	typedef vector<pair<string, JSONchild*> > vChildren;
	vChildren children;
	
};

class JSONarray : public JSON {
public:
	JSONarray() {};
	JSONarray(string);
	~JSONarray();
	void addChild(string);
	JSONobject* addObject(string = "");
	string str(int = 0);
	bool getChild(int, string*);
	
protected:
	typedef vector<JSONchild*> vChildren;
	vChildren children;
	
};

struct JSONchild {
	string value;
	JSONarray* array;
	JSONobject* object;
	JSONchild() {
		array = NULL;
		object = NULL;
		value = "";
	}
};

#endif