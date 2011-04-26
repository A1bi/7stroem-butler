#include <sstream>
#include <stdlib.h>
#include "http.h"
#include "httprequest.h"

// parses HTTP request
HTTPrequest::HTTPrequest(string* d) {
	stringstream data, tmpUri;
	string value;
	bool gotMethod = false, gotUri = false;
	char digit;
	
	// copy given data into data stream so we can work with it
	data << *d;
	
	// go through all digits of this line but quit after first line because we only need the first here
	while (data.get(digit) && digit != '\n') {
		
		// a space seperates the values so at this point a value must be complete
		if (digit == ' ') {
			// method not set yet -> this must be the method (comes first)
			if (!gotMethod) {
				method = value;
				gotMethod = true;
				// uri not set yet -> this must be the uri (comes second)
			} else if (!gotUri) {
				// storing the char in a temp var - uri has yet to be parsed
				tmpUri << value;
				gotUri = true;
			}
			// reset value
			value.clear();
		
		// add to value
		} else {
			value += digit;
		}
		
	}
	
	
	// parse GET arguments from uri
	string key;
	// reset gotUri because we can reuse it here
	// here it means that we got the uri without the arguments
	gotUri = false;
	// also reset value to be sure its empty
	value.clear();

	while (tmpUri.get(digit)) {
		// a questionmark marks the end of the uri
		if (digit == '?') {
			gotUri = true;
		// part of uri
		} else if (!gotUri) {
			uri += digit;
		// equals sign marks end of key
		} else if (digit == '=') {
			key = value;
			value.clear();
		// & sign separates two arguments so this argument ends here
		} else if (digit == '&') {
			// copy argument to get variable
			get[key] = value;
			key.clear();
			value.clear();
		// nothing from the above so just add to value
		} else {
			value += digit;
		}

	}
	// also save last argument which may not got saved because it didn't end with &
	if (gotUri && !key.empty()) {
		get[key] = value;
	}
	
	
	// parse the rest of the header
	parseHeader(data);
	
}

HTTPrequest::HTTPrequest(const string h, const string u) : host(h), uri(u), method("GET") {}

// generates request header
string HTTPrequest::generateHeader() {
	stringstream r;
	
	// first line with method and requested uri
	r << method << " " << uri << " HTTP/1.1\n";
	// host line
	r << "Host: " << host << endl;
	// header vars
	r << generateHeaderVars();
	if (method == "POST") {
		r << "Content-Type: application/x-www-form-urlencoded" << endl;
		r << "Content-Length: " << body.size() << endl;
	}
	// add empty line before body
	r << "\n";
	
	return r.str();
}

HTTPresponse* HTTPrequest::execute() {
	// already executed
	//if (executed) return false;
	
	// connect to http server
	Socket receiver;
	receiver.create();
	if (!receiver.connect(host, 80)) {
		// could not connect
		return NULL;
	}
	
	// generate header to send to server
	string request = generateHeader();

	// add body and replace last & with final break
	request += body;
	request.replace(request.size()-1, 1, "\n");

	// send header and body
	receiver.send(request);
	
	// get response
	string rawResponse;
	// get the whole response (header and body) or only the first chunk of it if chunking is enabled by the server
	int contentGot = receiver.recv(rawResponse);
	stringstream responseStream;
	responseStream << rawResponse;
	
	// create response object and parse it
	HTTPresponse* response = new HTTPresponse(responseStream);
	
	string line;

	// got content-length -> check if we already received the complete body and try to get the rest if we didn't
	if (response->getHeaderValue("Content-Length") != "") {
		int contentLeft;
		copyLines(response, &responseStream);
		
		// calculate content left to be received
		contentLeft = atoi(response->getHeaderValue("Content-Length").c_str());

		while (true) {
			contentLeft -= contentGot;
			if (contentLeft > 0) {
				contentGot = receiver.recv(rawResponse, contentLeft);
			} else {
				break;
			}
			responseStream.str(rawResponse);
			copyLines(response, &responseStream);
		}
		
	
	// check if chunking is enabled
	} else if (response->getHeaderValue("Transfer-Encoding") == "chunked") {
		
		int chunkSize, chunkSizeGot = 0, chunkSizeNext = 0;
		stringstream chunkSizeHex;
		while (true) {
			
			// new chunk -> get chunk size from first line
			if (chunkSizeGot == 0) {
				getline(responseStream, line);
				chunkSizeHex.str("");
				chunkSizeHex.clear();
				chunkSizeHex << hex << line;
				chunkSizeHex >> chunkSize;
			}

			usleep(10000);
			// last and terminating chunk is empty so stop here
			if (chunkSize == 0) {
				break;
			}

			// copy all lines of the body
			copyLines(response, &responseStream);
			// decrease because we need to ignore line feed from recv()
			chunkSizeGot--;

			// chunk not yet completely received
			if (chunkSizeGot < chunkSize) {
				chunkSizeNext = chunkSize - chunkSizeGot;
			// chunk complete
			} else {
				chunkSizeGot = 0;
				chunkSizeNext = 8;
			}

			// receive rest of the chunk or the beginning of the next
			chunkSizeGot = receiver.recv(rawResponse, chunkSizeNext+2);
			responseStream.str(rawResponse);

		}
		
			
	} else {
		// is not chunked so just copy the body
		copyLines(response, &responseStream);
	}
	
	
	// close connection to server
	receiver.close();
	
	return response;
	
}

void HTTPrequest::setGet(string key, string value) {
	get[key] = value;
}

void HTTPrequest::setPost(string key, string value) {
	if (method != "POST") {
		method = "POST";
	}
	body += key + "=" + urlEncode(value) + "&";
}

string HTTPrequest::getGet(string key) {
	return get[key];
}

/*string HTTPrequest::getPost(string key) {
	return post[key];
}*/

void HTTPrequest::copyLines(HTTPresponse* response, stringstream* lines) {
	string line;
	while (getline(*lines, line)) {
		line += "\n";
		*response << line;
	}
}