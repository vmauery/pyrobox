#include <model.hpp>
#include <db.hpp>
#include <util.hpp>
#include <sstream>
#include <iostream>

using namespace std;
//////////////////////////////////////////////////////////////////////////
// class model
/////////////////////////////////////////////////////////////////////////
db::wptr model::_wdb;
const char model::_nothing_[] = "";

void model::opendb() {
	_db = _wdb.lock();
	if (!_db) {
		_db.reset(new db(get_conf("db_file")));
		_wdb = _db;
	}
}

model::model() {
	opendb();
}

model::model(long int i) {
	char sql[128];
	opendb();
	sprintf(sql, "select * from %s where id=%ld", this->table_name(), i);
	_db->execute(sql, _values);
}

model::model(const db::Result& vals) {
	opendb();
	_values = vals;
}

long int model::id() {
	long int x;
	istringstream i(_values["id"]);
	if (!(i >> x))
     	return 0;
	return x;
}

list<model::ptr> model::fetch_all(const string& type) {

	if (type == "static_dhcp") {
		return static_dhcp::all();
	}
	if (type == "deny_dhcp") {
		return deny_dhcp::all();
	}
	return list<model::ptr>();
}

model::ptr model::factory(const string& type, const db::Result& vals) {

	if (type == "static_dhcp") {
		return static_dhcp::create(vals);
	}
	if (type == "deny_dhcp") {
		return deny_dhcp::create(vals);
	}
	return model::ptr();
}

bool model::save() {
	const vector<string>& _names = this->names();
	stringstream ss;
	if (id()) {
		// update
		ss << "UPDATE " << table_name() << " SET ";
		for (vector<string>::const_iterator i=_names.begin();;) {
			if (*i == "id") { i++; continue; }
			// FIXME: escape the values before inserting into db (' and \ must be escaped)
			ss << *i << "='" << _values[*i] << "'";
			if (++i == _names.end()) break;
			ss << ", ";
		}
		ss << " WHERE id=" << _values["id"];
		_db->execute(ss.str());
	} else {
		// insert
		long int id;
		char idstr[128];
		ss << "INSERT INTO " << table_name() << " (";
		for (vector<string>::const_iterator i=_names.begin();;) {
			if (*i == "id") { i++; continue; }
			ss << *i;
			if (++i == _names.end()) break;
			ss << ", ";
		}
		ss << ") VALUES (";
		for (vector<string>::const_iterator i=_names.begin();;) {
			if (*i == "id") { i++; continue; }
			// FIXME: escape the values before inserting into db (' and \ must be escaped)
			ss << "'" << _values[*i] << "'";
			if (++i == _names.end()) break;
			ss << ", ";
		}
		ss << ")";
		_db->execute(ss.str(), id);
		sprintf(idstr, "%ld", id);
		_values["id"] = idstr;
	}
	return true;
}
bool model::delete_row() {
	const vector<string>& _names = this->names();
	stringstream ss;
	if (id()) {
		// update
		ss << "DELETE FROM " << table_name()
		   << " WHERE id=" << _values["id"];
		_db->execute(ss.str());
	} else {
		return false;
	}
	return true;
}

vector<string> model::values() const {
	vector<string> n;
	for (db::Result::const_iterator i=_values.begin(); i!=_values.end(); i++) {
		n.push_back(i->second);
	}
	return n;
}

// template<class model_class>::all()
#define model_all(model_class)                                       \
list<model::ptr> model_class::all() {                           \
	model_class v;                                                   \
	list<model::ptr> ret;                                       \
	db::Results::iterator i;                                         \
	db::Results results;                                             \
	char sql[128];                                                   \
	sprintf(sql, "select * from %s", v.table_name());        \
	v._db->execute(sql, results);                                    \
	for (i=results.begin(); i!=results.end(); i++) {                 \
		model::ptr r(new model_class(*i));                           \
		ret.push_back(r);                                            \
	}                                                                \
	return ret;                                                      \
}

// template<class model_class>::create(const db::Result& vals)
#define model_create(model_class)                                        \
model_class::ptr model_class::create(const db::Result& vals) {     \
	model_class::ptr ret(new model_class(vals));                   \
	return ret;                                                    \
}


//////////////////////////////////////////////////////////////////////////
// class static_dhcp
/////////////////////////////////////////////////////////////////////////
const char static_dhcp::_table_name[] = "app_staticdhcp";

const vector<string>& static_dhcp::names() const {
	static vector<string> _names;
	if (_names.size() == 0) {
		_names.push_back("id");
		_names.push_back("hostname");
		_names.push_back("ip_addr");
		_names.push_back("mac_addr");
	}
	return _names;
}

model_all(static_dhcp)
model_create(static_dhcp)

string static_dhcp::json() {
	stringstream ss;
	ss << "{ "
	   << "\"id\": " << id() << ", "
	   << "\"hostname\": \"" << hostname() << "\", "
	   << "\"ip_addr\": \"" << ip_addr() << "\", "
	   << "\"mac_addr\": \"" << mac_addr() << "\", "
	   << " }";
	return ss.str();
}


//////////////////////////////////////////////////////////////////////////
// class deny_dhcp
/////////////////////////////////////////////////////////////////////////
const char deny_dhcp::_table_name[] = "app_denydhcp";

const vector<string>& deny_dhcp::names() const {
	static vector<string> _names;
	if (_names.size() == 0) {
		_names.push_back("id");
		_names.push_back("mac_addr");
		_names.push_back("hostname");
	}
	return _names;
}

model_all(deny_dhcp)
model_create(deny_dhcp)

string deny_dhcp::json() {
	stringstream ss;
	ss << "{ "
	   << "\"id\": " << id() << ", "
	   << "\"mac_addr\": \"" << mac_addr() << "\", "
	   << "\"hostname\": \"" << hostname() << "\", "
	   << " }";
	return ss.str();
}


//////////////////////////////////////////////////////////////////////////
// class variable
/////////////////////////////////////////////////////////////////////////
const char variable::_table_name[] = "app_variable";

const vector<string>& variable::names() const {
	static vector<string> _names;
	if (_names.size() == 0) {
		_names.push_back("id");
		_names.push_back("vgroup");
		_names.push_back("name");
		_names.push_back("value");
	}
	return _names;
}

variable::variable(const string& vg, const string& n) {
	char sql[512];
	sprintf(sql, "select * from %s where vgroup='%s' and name='%s'",
		table_name(), vg.c_str(), n.c_str());
	if (_db->execute(sql, _values) != SQLITE_OK) {
		info("variable " << vg << ":" << n << " not set");
		vgroup(vg);
		name(n);
	}
	info(_values["vgroup"] << ", "<< _values["name"] << ", " << _values["value"]);
}

list<model::ptr> variable::all(const string& form) {
	variable v;
	list<model::ptr> ret;
	db::Results::iterator i;
	db::Results results;
	char sql[512];
	sprintf(sql, "select * from %s where vgroup='%s'",
		v.table_name(), form.c_str());
	v._db->execute(sql, results);
	for (i=results.begin(); i!=results.end(); i++) {
		model::ptr r(new variable(*i));
		ret.push_back(r);
	}
	return ret;
}

string variable::json() {
	stringstream ss;
	ss << "\"" << name() << "\": \""
	   << value() << "\"";
	return ss.str();
}


std::string variable::get(const std::string& vg, const std::string& n) {
	variable v(vg, n);
	return v.value();
}

void variable::set(const std::string& vg, const std::string &n, const std::string &v) {
	variable sv(vg, n);
	sv.value(v);
	sv.save();
}






//////////////////////////////////////////////////////////////////////////
// _TEST_
/////////////////////////////////////////////////////////////////////////
#ifdef _TEST_
#include <list>
#include <iostream>
int main(int argc, char *argv[]) {
	basic_string<char> response;
	list<static_dhcp>::iterator rowiter;
	list<static_dhcp> statichosts = static_dhcp::all();
	stringstream ss;
	ss << "[\n";
	for (rowiter=statichosts.begin(); rowiter!=statichosts.end(); ) {
		ss << rowiter->json();
		if (++rowiter != statichosts.end()) {
			ss << ",\n";
		}
	}
	ss << "\n]";
	response = ss.str();
	cout << "Content-Type: application/json\r\n"
		<< "Expires: Wed, 14 Jan 2000 05:45:48 GMT\r\n"
		<< "Content-Length: " << response.length() << "\r\n\r\n"
		<< response;

	return 0;
}
#endif
