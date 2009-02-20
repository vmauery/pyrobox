#include <iostream>
#include <fstream>
#include <deque>
#include <boost/pointer_cast.hpp>

#include <form.hpp>
#include <util.hpp>
#include <model.hpp>

using namespace std;

static bool valid_string(const string& v) {

}

static bool valid_int(const string& v) {

}

static bool valid_float(const string& v) {

}

static bool valid_port(const string& v) {

}

static bool valid_ipv4(const string& v) {

}

static bool valid_ipv6(const string& v) {

}

static bool valid_mac(const string& v) {

}

static bool valid_hostname(const string& v) {

}

static bool valid_list(const string& v) {

}


const char* form_base::type_name() const {
	static const char *_type_names[] = {
			"form_element",
			"form_set",
			"field_set",
			"form",
			"button",
			"submit",
			"textarea",
			"textbox",
			"password",
			"multiple_choice",
			"select",
			"combobox",
			"radios",
			"checkbox",
			"checkboxes",
			"hidden",
			"file",
		};
	if (_type < form_base::element_count && _type > 0)
		return _type_names[_type];
	return NULL;
}

string form_base::render_attrs() const {
	std::stringstream ss;
	for (strmap::const_iterator i=_attrs.begin(); i!=_attrs.end();) {
		ss << "\"" << i->first << "\": \"" << i->second << "\"";
		if (++i != _attrs.end())
			ss << ", ";
	}
	return ss.str();
}

form_base::ptr form_base::factory(const std::string& type, const strmap& attrs, const pairlist& options) {
	if (type == "form")
		return form::create(attrs);
	if (type == "button")
		return button::create(attrs);
	if (type == "submit")
		return submit::create(attrs);
	if (type == "textarea")
		return textarea::create(attrs);
	if (type == "textbox")
		return textbox::create(attrs);
	if (type == "password")
		return password::create(attrs);
	if (type == "select")
		return select::create(attrs, options);
	if (type == "combobox")
		return combobox::create(attrs, options);
	if (type == "radios")
		return radios::create(attrs, options);
	if (type == "checkbox")
		return checkbox::create(attrs);
	if (type == "checkboxes")
		return checkboxes::create(attrs, options);
	if (type == "hidden")
		return hidden::create(attrs);
	if (type == "file")
		return file::create(attrs);
	//throw form_base::invalid_type(type);
	error("invalid type: "<<type);
	return form_base::ptr();
}

/*
 * form_element class functions
 */
string form_element::render() const {
	stringstream ss;
	ss << "{ \"type\": \"" << form_base::type_name() << "\", "
	   << this->render_attrs() << " }";
	return ss.str();
}

/*
 * form_set class functions
 */
string form_set::render() const {
	stringstream ss;
	ss << "{ \"type\": \"" << this->type_name() << "\", " << render_attrs()
	   << ", \"elements\": [\n";

	// render all the children
	vector<form_base::ptr>::const_iterator i = _children.begin();
	while (i != _children.end()) {
		ss << (*i)->render();
		if (++i != _children.end()) {
			ss << ",\n";
		}
	}
	ss << "]\n}\n";
	return ss.str();
}

bool form_set::valid() const {
	bool all_valid = true;
	// render all the children
	vector<form_base::ptr>::const_iterator i = _children.begin();
	while (i != _children.end()) {
		all_valid &= (*i)->valid();
		i++;
	}
	return all_valid;
}


/*
 * field_set class functions
 */
field_set::ptr field_set::create(const strmap& attrs) {
	field_set::ptr ret(new field_set(attrs));
	return ret;
}


/*
 * button class functions
 */
button::ptr button::create(const strmap& attrs) {
	button::ptr ret(new button(attrs));
	return ret;
}

bool button::valid() const {
	return true;
}


/*
 * submit class functions
 */
submit::ptr submit::create(const strmap& attrs) {
	submit::ptr ret(new submit(attrs));
	return ret;
}

bool submit::valid() const {
	return true;
}


/*
 * textarea class functions
 */
textarea::ptr textarea::create(const strmap& attrs) {
	textarea::ptr ret(new textarea(attrs));
	return ret;
}

bool textarea::valid() const {
	return true;
}


/*
 * textbox class functions
 */
textbox::ptr textbox::create(const strmap& attrs) {
	textbox::ptr ret(new textbox(attrs));
	return ret;
}

bool textbox::valid() const {
	return true;
}


/*
 * password class functions
 */
password::ptr password::create(const strmap& attrs) {
	password::ptr ret(new password(attrs));
	return ret;
}

bool password::valid() const {
	return true;
}

/*
 * multiple_choice class functions
 */
string multiple_choice::render() const {
	stringstream ss;
	ss << "{ \"type\": \"" << this->type_name() << "\"," << render_attrs()
	   << ", \"options\": [\n";

	// render all the children
	pairlist::const_iterator i = _options.begin();
	while (i != _options.end()) {
		ss << "{\"label\":\"" << i->second << "\", \"value\":\"" << i->first << "\"}";
		if (++i != _options.end()) {
			ss << ",\n";
		}
	}
	ss << "]\n}\n";

	return ss.str();
}

bool multiple_choice::valid() const {
	// make sure that the choice is one of the options
	return true;
}

/*
 * select class functions
 */
select::ptr select::create(const strmap& attrs, const pairlist& options) {
	select::ptr ret(new select(attrs, options));
	return ret;
}

/*
 * combobox class functions
 */
combobox::ptr combobox::create(const strmap& attrs, const pairlist& options) {
	combobox::ptr ret(new combobox(attrs, options));
	return ret;
}


/*
 * radios class functions
 */
radios::ptr radios::create(const strmap& attrs, const pairlist& options) {
	radios::ptr ret(new radios(attrs, options));
	return ret;
}


/*
 * checkbox class functions
 */
checkbox::ptr checkbox::create(const strmap& attrs) {
	checkbox::ptr ret(new checkbox(attrs));
	return ret;
}

bool checkbox::valid() const {
	return true;
}


/*
 * checkboxes class functions
 */
checkboxes::ptr checkboxes::create(const strmap& attrs, const pairlist& options) {
	checkboxes::ptr ret(new checkboxes(attrs, options));
	return ret;
}


/*
 * file class functions
 */
file::ptr file::create(const strmap& attrs) {
	file::ptr ret(new file(attrs));
	return ret;
}

bool file::valid() const {
	return true;
}


/*
 * hidden class functions
 */
hidden::ptr hidden::create(const strmap& attrs) {
	hidden::ptr ret(new hidden(attrs));
	return ret;
}

bool hidden::valid() const {
	return true;
}

/*
 * form class functions
 */
form::ptr form::create(const string& name) {
	string form_file_name = get_conf("form_dir") + "/" + name;
	ifstream form_file;
	form_file.open(form_file_name.c_str(), ios::binary);
	if (!form_file) {
		//throw form::does_not_exist(name);
		error("form does not exist: " << name);
	}
	form::ptr root;

	char *cline = new char[4096];
	string line;
	int lineno = 0;
	deque<string> stack;
	deque<form_set::ptr> pstack;
	form_base::ptr fel;
	while (!form_file.eof()) {
		form_file.getline(cline, 4096);
info("parsing line: " << cline);
		lineno++;
		line = cline;
		size_t tok = line.find(':');
		if (tok == string::npos) {
			if (line.length() == 0 || line[0] == '#')
				continue;
			//throw parse_error("line without a ':'", cline, lineno);
			error("parse error");
		}
		string value, name = trim(line.substr(0, tok));
info("name is '"<<name<<"'");
		if (++tok < line.length())
			value = trim(line.substr(tok));
		if (value.length() == 0) {
			// start of an indented stanza
			if (name == "options") {
				if (!boost::dynamic_pointer_cast<multiple_choice>(fel)) {
					//throw parse_error("options tag not valid here", cline, lineno);
					error("parse error");
				}
			} else if (name == "elements") {
				// try a cast to a form_set
				form_set::ptr fset = boost::dynamic_pointer_cast<form_set>(fel);
				if (!fset) {
					//throw parse_error("element tag not valid here", cline, lineno);
			error("parse error");
				}
				pstack.push_back(fset);
			} else {
				try {
					fel = form_base::factory(name);
					info("new " << name << " element (" << fel.get() << ")");
				} catch (form::invalid_type e) {
					//throw parse_error("invalid type", cline, lineno);
					error("parse error");
				}
				if (!root) {
					if (!(root = boost::dynamic_pointer_cast<form>(fel)))
						//throw parse_error("form not root element", cline, lineno);
						error("parse error");
				} else {
					if (!pstack.empty()) {
						fel->parent(pstack.back());
						pstack.back()->add(fel);
					} else {
						//throw parse_error("child element not wrapped in 'elements' tag", cline, lineno);
						error("parse error");
					}
				}
			}
			stack.push_back(name);
			continue;
		}
		if (name == "end") {
			if (stack.back() != value) {
				//throw parse_error("non-matching end block", cline, lineno);
				error("parse error");
			}
			if (value == "elements") {
				fel = pstack.back();
				pstack.pop_back();
			}
			stack.pop_back();
			// read a line
			continue;
		}
		if (stack.back() == "options") {
			info("adding option " << name << " -> '" << value << "' to " << fel.get());
			boost::static_pointer_cast<multiple_choice>(fel)->option(name, value);
		} else {
			info("adding " << name << " -> '" << value << "' to " << fel.get());
			fel->attr(name, value);
		}
	}
	delete [] cline;
	form_file.close();
	return root;
}

form::ptr form::create(const strmap& attrs) {
	form::ptr ret(new form(attrs));
	return ret;
}

string form::render_values() const {
	stringstream ss;
	list<model::ptr>::iterator rowiter;
	list<model::ptr> models = variable::all(realm());
	info("render_values for " << realm());
	ss << "{\n";
	for (rowiter=models.begin(); rowiter!=models.end(); ) {
		ss << (*rowiter)->json();
		if (++rowiter != models.end()) {
			ss << ",\n";
		}
	}
	ss << "\n},\n";
	return ss.str();
}


string form::submit(const strmap& post) const {
	stringstream ss;
	info("form::submit(" << name() << ")");
	strmap validated = validate(post);
	string form_realm = realm();

	for (strmap::const_iterator i=validated.begin(); i!=validated.end(); i++) {
		variable::set(form_realm, i->first, i->second);
	}
	return ss.str();
}

strmap form::validate(const map<string,string>& values) const {
	strmap valid_values = values;
	// filter posted values for valid names
	// validate valid names against validators specified by json text
	return valid_values;
}

string form::realm() const {
	strmap::const_iterator _realm = _attrs.find("realm");
	if (_realm != _attrs.end())
		return _realm->second;
	return name();
}

