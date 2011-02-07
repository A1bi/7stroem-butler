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
// add child
void JSONobject::addChild(string key, string value) {
	// create child
	JSONchild* child = new JSONchild;
	// assign string to value
	child->value = value;
	// push child into vector
	pushChild(key, child);
}

// add array
JSONarray* JSONobject::addArray(string key) {
	// create json array
	JSONarray* array = new JSONarray;
	JSONchild* child = new JSONchild;
	// assign pointer to object
	child->array = array;
	pushChild(key, child);
	// return array pointer
	return array;
}

// add json object
JSONobject* JSONobject::addObject(string key) {
	// create object pointer
	JSONobject* object = new JSONobject;
	JSONchild* child = new JSONchild;
	// assign pointer
	child->object = object;
	// push child into vector
	pushChild(key, child);
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

// pushes child into vector
void JSONobject::pushChild(string key, JSONchild* child) {
	// create a pair which stores key and JSONchild object
	pair<string, JSONchild*> pChild(key, child);
	// push into vector as last element
	children.push_back(pChild);
}

// destructor
JSONobject::~JSONobject() {
	// go through all children
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		// destroy json object or array in child
		delete vIter->second->object;
		delete vIter->second->array;
		// destroy child object
		delete vIter->second;
	}
}


// -- JSONarray --
// the only difference to JSONobject is the lack of a key for each child
// for the rest have a look at the comments from JSONobject above
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
JSONobject* JSONarray::addObject() {
	JSONobject* object = new JSONobject;
	JSONchild* child = new JSONchild;
	child->object = object;
	children.push_back(child);
	
	return object;
}

// destructor
JSONarray::~JSONarray() {
	// go through all children
	vChildren::iterator vIter;
	for (vIter = children.begin(); vIter != children.end(); ++vIter) {
		// destroy json object in child
		delete (*vIter)->object;
		// destroy child object
		delete *vIter;
	}
}