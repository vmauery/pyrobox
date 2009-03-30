#include <db.hpp>
#include <logging.hpp>
//#include <iostream>

using namespace std;

static int parse_rows(void *arg, int col_count, char **columns, char **col_names) {
	db::Results *results = (db::Results *)arg;
	db::Result row;
	char * null="";
	for (int i=0; i<col_count; i++) {
		// info(col_names[i] << " => " << (columns[i]?columns[i]:null) << ", ");
		row[col_names[i]] = (columns[i]?columns[i]:null);
	}
	results->push_back(row);
	return 0;
}

static int parse_row(void *arg, int col_count, char **columns, char **col_names) {
	db::Result *row = (db::Result *)arg;
	for (int i=0; i<col_count; i++) {
//		cout << col_names[i] << " => " << columns[i] << ", ";
		(*row)[col_names[i]] = columns[i];
	}
//	cout << endl;
	return 0;
}

db::db(const string &dbname) {
	int rc = sqlite3_open(dbname.c_str(), &handle);
	if (rc == SQLITE_OK) {
//		cout << "db " << dbname << " open (" << handle << ")" << endl;
	} else {
//		cout << "db " << dbname << " failed to open" << endl;
	}
}
db::~db() {
	sqlite3_close(handle);
//	cout << "db (" << handle << ") closed" << endl;
}

bool db::execute(const string &query) {
	char *errors = NULL;
	info("db (" << handle << "): " << query);
	if (sqlite3_exec(handle, query.c_str(), parse_row, NULL, &errors) != SQLITE_OK) {
		warn("\"" << query << "\": " << errors);
		sqlite3_free(errors);
		return false;
	}
	return true;
}

bool db::execute(const string &query, long int &id) {
	char *errors = NULL;
	info("db (" << handle << "): " << query);
	if (sqlite3_exec(handle, query.c_str(), parse_row, NULL, &errors) != SQLITE_OK) {
		warn("\"" << query << "\": " << errors);
		sqlite3_free(errors);
		return false;
	}
	id = (long int)sqlite3_last_insert_rowid(handle);
	return true;
}

bool db::execute(const string &query, db::Results &results) {
	char *errors = NULL;
	info("db (" << handle << "): " << query);
	if (sqlite3_exec(handle, query.c_str(), parse_rows, &results, &errors) != SQLITE_OK) {
		warn("\"" << query << "\": " << errors);
		sqlite3_free(errors);
		return false;
	}
	return true;
}

bool db::execute(const string &query, db::Result &result) {
	char *errors = NULL;
	info("db (" << handle << "): " << query);
	if (sqlite3_exec(handle, query.c_str(), parse_row, &result, &errors) != SQLITE_OK) {
		warn("\"" << query << "\": " << errors);
		sqlite3_free(errors);
		return false;
	}
	return true;
}

#ifdef _TEST_
#include <iostream>
#include <sstream>

int main(int argc, char *argv) {
/*
	db d("pyrobox.db");
	deque<vector<string> > rows;
	vector<string> cols;
	d.execute("select * from app_staticdhcp", rows, cols);
	vector<string>::iterator i;
	deque<vector<string> >::iterator j;
	for (i=cols.begin(); i != cols.end(); i++) {
		cout << *i << "|";
	}
	cout << endl;
	for (j=rows.begin(); j != rows.end(); j++) {
		for (i=j->begin(); i != j->end(); i++) {
			cout << *i << "|";
		}
		cout << endl;
	}
*/
	string response;
	stringstream ss;
	db d("pyrobox.db");
	db::Results statichosts;
	d.execute("select * from app_staticdhcp", statichosts);
	db::Results::iterator rowiter;
	db::Result::iterator coliter;
	ss << "[\n";
	for (rowiter=statichosts.begin(); rowiter!=statichosts.end(); ) {
		ss << "{ ";
		for (coliter=rowiter->begin(); coliter!=rowiter->end(); ) {
			ss << coliter->first << ":" << "\"" << coliter->second << "\"";
			if (++coliter != rowiter->end()) {
				ss << ", ";
			}
		}
		ss << " }";
		if (++rowiter != statichosts.end()) {
			ss << ",\n";
		}
	}
	ss << "\n]";
	response = ss.str();
	cout << response << endl;
	return 0;
}
#endif
