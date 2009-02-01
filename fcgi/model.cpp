#include "model.hpp"
#include "db.hpp"
#include <sstream>
#include <iostream>

//////////////////////////////////////////////////////////////////////////
// class model
/////////////////////////////////////////////////////////////////////////
db::wptr model::_wdb;
const char model::_nothing_[] = "";

void model::opendb() {
	_db = _wdb.lock();
	if (!_db) {
		_db.reset(new db("/home/vhmauery/local/pyrobox/pyrobox.db"));
		_wdb = _db;
	}
}

model::model() {
	opendb();
}

model::model(int i) {
	char sql[128];
	opendb();
	sprintf(sql, "select * from %s where id=%d", this->table_name(), i);
	_db->execute(sql, values);
}

model::model(db::Result& vals) {
	opendb();
	values = vals;
}

int model::id() {
	int x;
	std::istringstream i(values["id"]);
	if (!(i >> x))
     	return 0;
	return x;
}

std::list<model::ptr> model::fetch_all(const std::string& type) {

	if (type == "static_dhcp") {
		return static_dhcp::all();
	}
	return std::list<model::ptr>();
}

// template<class model_class>::all()
#define model_all(model_class)                                       \
std::list<model::ptr> model_class::all() {                           \
	model_class v;                                                   \
	std::list<model::ptr> ret;                                       \
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


//////////////////////////////////////////////////////////////////////////
// class static_dhcp
/////////////////////////////////////////////////////////////////////////
const char static_dhcp::_table_name[] = "app_staticdhcp";

model_all(static_dhcp)

std::string static_dhcp::json() {
	std::stringstream ss;
	ss << "{ "
	   << "id: " << id() << ", "
	   << "hostname: \"" << hostname() << "\", "
	   << "ip_addr: \"" << ip_addr() << "\", "
	   << "mac_addr: \"" << mac_addr() << "\", "
	   << " }";
	return ss.str();
}


//////////////////////////////////////////////////////////////////////////
// class variable
/////////////////////////////////////////////////////////////////////////
const char variable::_table_name[] = "app_variable";

std::list<model::ptr> variable::all(const std::string& form) {
	variable v;
	std::list<model::ptr> ret;
	db::Results::iterator i;
	db::Results results;
	char sql[128];
	sprintf(sql, "select * from %s where form='%s'",
		v.table_name(), form.c_str());
	v._db->execute(sql, results);
	for (i=results.begin(); i!=results.end(); i++) {
		model::ptr r(new variable(*i));
		ret.push_back(r);
	}
	return ret;
}

std::string variable::json() {
	std::stringstream ss;
	ss << "{ "
	   << name() << ": \""
	   << value() << "\", "
	   << " }";
	return ss.str();
}









//////////////////////////////////////////////////////////////////////////
// _TEST_
/////////////////////////////////////////////////////////////////////////
#ifdef _TEST_
#include <list>
#include <iostream>
int main(int argc, char *argv[]) {
	std::basic_string<char> response;
	std::list<static_dhcp>::iterator rowiter;
	std::list<static_dhcp> statichosts = static_dhcp::all();
	std::stringstream ss;
	ss << "[\n";
	for (rowiter=statichosts.begin(); rowiter!=statichosts.end(); ) {
		ss << rowiter->json();
		if (++rowiter != statichosts.end()) {
			ss << ",\n";
		}
	}
	ss << "\n]";
	response = ss.str();
	std::cout << "Content-Type: application/json\r\n"
		<< "Expires: Wed, 14 Jan 2000 05:45:48 GMT\r\n"
		<< "Content-Length: " << response.length() << "\r\n\r\n"
		<< response;

	return 0;
}
#endif
