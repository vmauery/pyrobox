#include <util.hpp>
#include <map>
#include <iostream>
#include <fstream>

using namespace std;

string check_url(string url) {
	return url;
}

string html_entities(const string& encode);

void lower_case(string &s) {
	transform(s.begin(), s.end(), s.begin(), (int(*)(int))tolower);
}

// breaks str up on any char in delimiter
vector<string> explode(const string& delimiter, const string& str);

string ltrim(const string &s) {
	return s.substr(s.find_first_not_of(" \t\r\n"));
}

string rtrim(const string &s) {
	size_t pos = s.find_last_not_of(" \t\r\n");
	if (pos != string::npos)
		pos++;
	return s.substr(0, pos);
}

string trim(const string &s) {
	size_t bpos = s.find_first_not_of(" \t\r\n");
	size_t epos = s.find_last_not_of(" \t\r\n");
	if (epos != string::npos)
		epos++;
	if (bpos != 0)
		epos -= bpos;
	return s.substr(bpos, epos);
}

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

static map<string,string> conf_map;
void read_config_file(const string& cfile) {
	// set some reasonable defaults
	conf_map["db_file"] = "/var/lib/pyrobox/pyrobox.sqlite";
	conf_map["form_dir"] = "/var/lib/pyrobox/forms";
	conf_map["log_file"] = "/tmp/fcgi.log";

	ifstream cf(cfile.c_str(), ifstream::in);

	while (cf.good()) {
		string line;
		getline(cf, line);
		line = line.substr(0, line.find('#'));
		info("conf line: "<<line);
		vector<string> nv = explode("=", line);
		if (nv.size() > 1) {
			conf_map[trim(nv[0])] = trim(nv[1]);
			info("conf_map[" << trim(nv[0]) << "] = " << trim(nv[1]));
		} else if (nv.size() == 1) {
			conf_map[trim(nv[0])] = "";
		}
	}
	cf.close();
}
static const string empty;
const string& get_conf(const string& cid) {
	map<string,string>::iterator ival;
	if ((ival = conf_map.find(cid)) != conf_map.end()) {
		return ival->second;
	}
	return empty;
}
