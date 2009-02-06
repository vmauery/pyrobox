#ifndef _DB_HPP_
#define _DB_HPP_

#include <sqlite3.h>
#include <map>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class db {
	public:
		typedef boost::shared_ptr<db> ptr;
		typedef boost::weak_ptr<db> wptr;
		typedef std::map<std::string,std::string> Result;
		typedef std::list<Result> Results;

	public:
		db(const std::string &dbname);
		~db();
		// execute, discard any results
		bool execute(const std::string &query);
		// execute, return insert id
		bool execute(const std::string &query, long int &id);
		// execute, return all matching rows
		bool execute(const std::string &query, Results &results);
		// execute, return the last matched row
		bool execute(const std::string &query, Result &result);

	protected:
		sqlite3 *handle;
};

#endif
