#ifndef _MODEL_HPP_
#define _MODEL_HPP_

#include <string>
#include <boost/shared_ptr.hpp>

#include "db.hpp"

//////////////////////////////////////////////////////////////////////////
// class model
/////////////////////////////////////////////////////////////////////////
class model {
	public:
		typedef boost::shared_ptr<model> ptr;

	public:
		model();
		model(int i);
		model(db::Result& vals);
		virtual ~model() {}
		int id();
		virtual std::string json() = 0;
		static std::list<ptr> fetch_all(const std::string& type);
	
	protected:
		virtual const std::string& table_name() { return _nothing_; }

	private:
		void opendb();
		static std::string _nothing_;
		static db::wptr _wdb;

	protected:
		db::ptr _db;
		db::Result values;
};

//////////////////////////////////////////////////////////////////////////
// class static_dhcp
/////////////////////////////////////////////////////////////////////////
class static_dhcp : public model {
	public:
		static_dhcp() : model() {}
		static_dhcp(int i) : model(i) {}
		static_dhcp(db::Result& vals) : model(vals) {}
		virtual ~static_dhcp() {}
		static std::list<model::ptr> all();

	protected:
		virtual const std::string& table_name() { return _table_name; }

	private:
		static const std::string _table_name;
	
	// variable access interface
	public:
		virtual std::string json();
		std::string& hostname() { return values["hostname"]; }
		void hostname(std::string& value) { values["hostname"] = value; }
		std::string& ip_addr() { return values["ip_addr"]; }
		void ip_addr(std::string& value) { values["ip_addr"] = value; }
		std::string& mac_addr() { return values["mac_addr"]; }
		void mac_addr(std::string& value) { values["mac_addr"] = value; }
};

//////////////////////////////////////////////////////////////////////////
// class variable
/////////////////////////////////////////////////////////////////////////
class variable : public model {
	public:
		variable() : model() {}
		variable(int i) : model(i) {}
		variable(db::Result& vals) : model(vals) {}
		virtual ~variable() {}
		static std::list<model::ptr> all(const std::string& form_name);

	protected:
		virtual const std::string& table_name() { return _table_name; }

	private:
		static const std::string _table_name;
	
	// variable access interface
	public:
		virtual std::string json();
		std::string& form() { return values["form"]; }
		void form(std::string& value) { values["form"] = value; }
		std::string& name() { return values["name"]; }
		void name(std::string& value) { values["name"] = value; }
		std::string& value() { return values["value"]; }
		void value(std::string& value) { values["value"] = value; }
};

#endif
