#include "db.hpp"
//#include <iostream>

int parse_rows(void *arg, int col_count, char **columns, char **col_names) {
	db::Results *results = (db::Results *)arg;
	db::Result row;
	for (int i=0; i<col_count; i++) {
//		std::cout << col_names[i] << " => " << columns[i] << ", ";
		row[col_names[i]] = columns[i];
	}
//	std::cout << std::endl;
	results->push_back(row);
	return 0;
}

int parse_row(void *arg, int col_count, char **columns, char **col_names) {
	db::Result *row = (db::Result *)arg;
	for (int i=0; i<col_count; i++) {
//		std::cout << col_names[i] << " => " << columns[i] << ", ";
		(*row)[col_names[i]] = columns[i];
	}
//	std::cout << std::endl;
	return 0;
}

db::db(const std::string &dbname) {
	int rc = sqlite3_open(dbname.c_str(), &handle);
	if (rc == SQLITE_OK) {
//		std::cout << "db " << dbname << " open (" << handle << ")" << std::endl;
	} else {
//		std::cout << "db " << dbname << " failed to open" << std::endl;
	}
}
db::~db() {
	sqlite3_close(handle);
//	std::cout << "db (" << handle << ") closed" << std::endl;
}

bool db::execute(const std::string &query) {
	char *errors = NULL;
//	std::cout << "db (" << handle << "): " << query << std::endl;
	sqlite3_exec(handle, query.c_str(), parse_row, NULL, &errors);
	if (errors) {
		sqlite3_free(errors);
		return false;
	}
	return true;
}

bool db::execute(const std::string &query, db::Results &results) {
	char *errors = NULL;
//	std::cout << "db (" << handle << "): " << query << std::endl;
	sqlite3_exec(handle, query.c_str(), parse_rows, &results, &errors);
	if (errors) {
		sqlite3_free(errors);
		return false;
	}
	return true;
}

bool db::execute(const std::string &query, db::Result &result) {
	char *errors = NULL;
//	std::cout << "db (" << handle << "): " << query << std::endl;
	sqlite3_exec(handle, query.c_str(), parse_row, &result, &errors);
	if (errors) {
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
	std::deque<std::vector<std::string> > rows;
	std::vector<std::string> cols;
	d.execute("select * from app_staticdhcp", rows, cols);
	std::vector<std::string>::iterator i;
	std::deque<std::vector<std::string> >::iterator j;
	for (i=cols.begin(); i != cols.end(); i++) {
		std::cout << *i << "|";
	}
	std::cout << std::endl;
	for (j=rows.begin(); j != rows.end(); j++) {
		for (i=j->begin(); i != j->end(); i++) {
			std::cout << *i << "|";
		}
		std::cout << std::endl;
	}
*/
	std::string response;
	std::stringstream ss;
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
	std::cout << response << std::endl;
	return 0;
}
#endif
