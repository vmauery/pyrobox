#include <iostream>
#include <fstream>

#include <form.hpp>
#include <util.hpp>
#include <model.hpp>

using namespace std;

form::ptr form::create(const string& name) {
	form::ptr ret = form::ptr(new form(name));
	return ret;
}

form::form(const string& name) : _name(name) {
	string form_file_name = get_conf("form_dir") + "/" + name;
	ifstream form_file;
	form_file.open(form_file_name.c_str(), ios::binary);
	if (!form_file) {
		throw form::does_not_exist(name);
	}
	form_file.seekg (0, ios::end);
	_data_len = form_file.tellg();
	form_file.seekg (0, ios::beg);
	_data.reset(new char[_data_len+1]);
	form_file.read(_data.get(), _data_len);
	_data.get()[_data_len] = 0;
	form_file.close();
}

void form::render_values(std::stringstream& ss) {
	std::list<model::ptr>::iterator rowiter;
	std::list<model::ptr> models = variable::all(_name);
	ss << "values: {\n";
	for (rowiter=models.begin(); rowiter!=models.end(); ) {
		ss << (*rowiter)->json();
		if (++rowiter != models.end()) {
			ss << ",\n";
		}
	}
	ss << "\n},\n";
}

string form::render() {
	stringstream ss;
	ss << _name << ": { form: ";
	ss.write(_data.get(), _data_len);
	ss << ", \n";
	render_values(ss);
	ss << "}, ";
	return ss.str();
}


string form::submit(const map<string,string>& post) {
	stringstream ss;
	info("form::submit(" << _name << ")");
	map<string,string> validated = validate(post);

	for (map<string,string>::iterator i=validated.begin(); i!=validated.end(); i++) {
		variable::set(_name, i->first, i->second);
	}
	return ss.str();
}

map<string,string> form::validate(const map<string,string>& values) {
	map<string,string> valid_values = values;
	// filter posted values for valid names
	// validate valid names against validators specified by json text
	return valid_values;
}

bool form::valid_string(const string& v) {

}

bool form::valid_int(const string& v) {

}

bool form::valid_float(const string& v) {

}

bool form::valid_port(const string& v) {

}

bool form::valid_ipv4(const string& v) {

}

bool form::valid_ipv6(const string& v) {

}

bool form::valid_mac(const string& v) {

}

bool form::valid_hostname(const string& v) {

}

bool form::valid_list(const string& v) {

}

