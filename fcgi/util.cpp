#include <util.hpp>

using namespace std;

// breaks str up on any char in delimiter
vector<string> explode(const string& delimiter, const string& str) {
	vector<string> ret;
	size_t start = 0, end = 0;
	info("explode("<<delimiter<<", "<<str<<")");
	while (str.length() > start) {
		info("length: " << str.length() << ", start: " << start << ", end: " << end);
		end = str.find_first_of(delimiter, start);
		if (end == string::npos) {
			here();
			ret.push_back(str.substr(start));
			break;
		}
		here();
		ret.push_back(str.substr(start, end-start));
		here();
		start = end + 1;
	}
	return ret;
}

