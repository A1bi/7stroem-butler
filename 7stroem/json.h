// json.h
#ifndef _JSON_H_
#define _JSON_H_
#include <string>
#include <vector>
using namespace std;

// JSON generating stuff

class JSONchild;
class JSONarray;

class JSON {
protected:
	string getTabs(int);
};

class JSONobject : public JSON {
public:
	void addChild(string, string);
	JSONarray* addArray(string);
	JSONobject* addObject(string);
	string str(int = 0);
	~JSONobject();
	
protected:
	void pushChild(string, JSONchild*);
	typedef vector<pair<string, JSONchild*> > vChildren;
	vChildren children;

	
};

class JSONarray : public JSON {
public:
	void addChild(string);
	JSONobject* addObject();
	string str(int = 0);
	~JSONarray();
	
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