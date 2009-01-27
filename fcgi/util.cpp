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

string html_entities(const string& encode) {
	string out;
	size_t len = encode.length();
	string::const_iterator i;
	for (i = encode.begin(); i!=encode.end(); i++) {
		switch (*i) {
		case '&':
			out.append("&amp;");
			break;
		case '<':
			out.append("&lt;");
			break;
		case '>':
			out.append("&gt;");
			break;
		case '\'':
			out.append("&#039;");
			break;
		case '"':
			out.append("&quot;");
			break;
		default:
			out.push_back(*i);
			break;
		}
	}
	return out;
}
