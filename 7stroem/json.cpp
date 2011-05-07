#include "json.h"

// -- JSON base class --
string JSON::getTabs(int depth) {
	string tabs;
	for (int i = 0; i < depth; i++) {
		tabs += "\t";
	}
	return tabs;
}


// -- JSONobject --
// parser
JSONobject::JSONobject(const string rawObject) {
	parse(rawObject);
}
void JSONobject::parse(const string rawObject) {
	if (rawObject == "") return;
	
	const boost::regex pattern("\"(.+?)\": *((\"(.+?)\")|(\\{(.+?)\\})|(\\[(.+?)\\]))");
	boost::smatch match;
	string::const_iterator st1 = rawObject.begin(), st2 = rawObject.end();
	
	while (boost::regex_search(st1, st2, match, pattern)) {		
		if (match[6] != "") {
			addObject(match[1], match[6]);
		} else if (match[8] != "") {
			addArray(match[1], match[8]);
		} else if (match[4] != "") {
			addChild(match[1], match[4]);
		}
		st1 = match[0].second;
	}
}

// add child
void JSONobject::addChild(string key, string value) {
	// create child
	JSONchild* child = newChild(key);
	// assign string to value
	child->value = value;
}

// add array
JSONarray* JSONobject::addArray(string key, string content) {
	// create json array
	JSONarray* array = new JSONarray(content);
	JSONchild* child = newChild(key);
	// assign pointer to object
	child->array = array;
	// return array pointer
	return array;
}

// add json object
JSONobject* JSONobject::addObject(string key, string content) {
	// create object pointer
	JSONobject* object = new JSONobject(content);
	JSONchild* child = newChild(key);
	// assign pointer
	child->object = object;
	// return json object pointer
	return object;
}

// generating the actual json string to send back to client
string JSONobject::str(int depth) {
	// { intoduces new object
	string JSONstr = "{";
	// entering new level so increase tabulator depth
	depth++;
	
	// go through all children
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		// add key to string first
		JSONstr += "\n" + getTabs(depth) + "\"" + vIter->first + "\": ";
		
		// child is a string
		if (!vIter->second->value.empty()) {
			JSONstr += "\"" + vIter->second->value + "\"";
		// child is an object
		} else if (vIter->second->object != NULL) {
			JSONstr += vIter->second->object->str(depth);
		// child is an array
		} else if (vIter->second->array != NULL) {
			JSONstr += vIter->second->array->str(depth);
		// no value given -> empty
		} else {
			JSONstr += "\"\"";
		}
		
		// add comma leading to the next child
		if (vIter+1 != children.end()) {
			JSONstr += ",";
		}
		
	}
	// leaving this level so decrease depth
	depth--;
	// new line, tabulators and closing }
	JSONstr += "\n" + getTabs(depth) + "}";
	
	return JSONstr;
}

// return a string child specified by key
string JSONobject::getChild(string key) {
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		if (vIter->first == key) {
			return vIter->second->value;
			break;
		}
	}
	return "";
}

// return an array child specified by key
JSONarray* JSONobject::getArray(string key) {
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		if (vIter->first == key) {
			return vIter->second->array;
			break;
		}
	}
	return NULL;
}

// pushes child into vector
JSONchild* JSONobject::newChild(string key) {
	// create a pair which stores key and JSONchild object
	pair<string, JSONchild*> pChild;
	pChild.first = key;
	JSONchild* child = new JSONchild;
	pChild.second = child;
	// push into vector as last element
	children.push_back(pChild);
	return child;
}

// destructor
JSONobject::~JSONobject() {
	// go through all children
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		// destroy child object
		delete vIter->second;
	}
}

// -- JSONarray --
// the only difference to JSONobject is the lack of a key for each child
// for the rest have a look at the comments from JSONobject above
JSONarray::JSONarray(string rawObject) {
	if (rawObject == "") return;
	
	// file pattern
	const boost::regex pattern("((\"(.+?)\")|(\\{(.+?)\\}))");
	boost::smatch match;
	string::const_iterator st1 = rawObject.begin(), st2 = rawObject.end();
	
	while (boost::regex_search(st1, st2, match, pattern)) {
		if (match[5] != "") {
			addObject(match[5]);
		} else if (match[3] != "") {
			addChild(match[3]);
		}
		st1 = match[0].second;
	}
}

string JSONarray::str(int depth) {
	string JSONstr = "[";
	depth++;
	
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		JSONstr += "\n" + getTabs(depth);
		if (!(*vIter)->value.empty()) {
			JSONstr += "\"" + (*vIter)->value + "\"";
		} else if ((*vIter)->object != NULL) {
			JSONstr += (*vIter)->object->str(depth);
		}
		
		JSONstr += ",";
	}
	
	JSONstr.erase(JSONstr.size()-1, 1);
	depth--;
	JSONstr += "\n" + getTabs(depth) + "]";
	
	return JSONstr;
}	

// add array
void JSONarray::addChild(string value) {
	JSONchild* child = new JSONchild;
	child->value = value;
	children.push_back(child);
}

// add object
JSONobject* JSONarray::addObject(string content) {
	JSONobject* object = new JSONobject(content);
	JSONchild* child = new JSONchild;
	child->object = object;
	children.push_back(child);
	
	return object;
}

// return a child specified by position
bool JSONarray::getChild(int i, string* value) {
	// check if not out of range
	if (i >= children.size()) {
		return false;
	}
	*value = children.at(i)->value;
	return true;
}

// destructor
JSONarray::~JSONarray() {
	// go through all children
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		// destroy child object
		delete *vIter;
	}
}