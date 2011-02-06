#include <string>
#include "json.h"
using namespace std;

class JSON;
class JSONarray;
struct JSONchild {
	string key;
	string value;
	JSONarray* array;
	JSON* object;
};