#include <sstream>
#include "http.h"
using namespace std;

// returns the value of a single  element
string HTTP::getHeaderValue(string key) {
	return headerVars[key];
}

// sets the value of a single header element
void HTTP::setHeaderValue(string key, string value) {
	headerVars[key] = value;
}

// gets all header vars with their values
// stops before body
void HTTP::parseHeader(stringstream& data) {
	
	// data is given as argument so we have to parse it
	stringstream lineStream, key, value;
	string line;
	// single digit of a line
	char digit;
	// indicates if we already have the complete key or header
	bool gotKey = false;
	
	// go through all digits of this line
	while (getline(data, line)) {
		
		// only a carriage return indicates an empty line -> end of header
		if (line == "\r") {
			break;
		
		// part of header
		} else {
			
			// go through all digits of this line
			lineStream << line;
			while (lineStream.get(digit)) {
				// skip carriage return
				if (digit == '\r') continue;
				// colon -> end of key
				if (digit == ':' && !gotKey) {
					gotKey = true;
				
				// part of value
				} else if (gotKey) {
					// skip first space after the colon
					if (digit != ' ' || value.str() != "") {
						value << digit;
					}
			
				// part of key
				} else {
					key << digit;
				}
			}
			
			// end of line and also got a key so now set value for this key
			if (gotKey) {
				// cookie have to be stored separately
				if (key.str() == "Set-Cookie") {
					rawCookies.push_back(value.str());
				} else {
					headerVars[key.str()] = value.str();
				}
				gotKey = false;
			}
			
			// clear vars for next line
			key.str("");
			key.clear();
			value.str("");
			value.clear();
			lineStream.str("");
			lineStream.clear();
		
		}
	
	}
	
}

string HTTP::generateHeaderVars() {
	stringstream r;
	// header vars
	map<string, string>::iterator vIter;
	for (vIter = headerVars.begin(); vIter != headerVars.end(); ++vIter) {
		r << vIter->first << ": " << vIter->second << "\n";
	}
	return r.str();
}

string HTTP::urlEncode(const string &content) {
	
    string escaped;
	const int max = content.length();
	char digit, dig1, dig2;
	
	for (int i = 0; i < max; i++) {
		digit = content[i];
		
		if ((48 <= digit && digit <= 57) || // 0-9
			(65 <= digit && digit <= 90) || // ABC...XYZ
			(97 <= digit && digit <= 122) || // abc...xyz
			(digit == '~' || digit == '-' || digit == '_' || digit == '.')) {
			escaped.append(&digit, 1);
		
		} else {
			escaped += "%";
			// convert char 255 to string "FF"
			dig1 = (digit&0xF0) >> 4;
			dig2 = (digit&0x0F);
			
			if (0 <= dig1 && dig1 <= 9) dig1 += 48;    // 0,48 in ascii
			if (10 <= dig1 && dig1 <= 15) dig1 += 65-10; // A,65 in ascii
			if (0 <= dig2 && dig2 <= 9) dig2 += 48;
			if (10 <= dig2 && dig2 <= 15) dig2 += 65-10;
			
			escaped.append(&dig1, 1);
			escaped.append(&dig2, 1);
		}
		
	}
	
	return escaped;

}