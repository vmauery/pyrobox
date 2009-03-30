#ifndef _MODEL_HPP_
#define _MODEL_HPP_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "db.hpp"

//////////////////////////////////////////////////////////////////////////
// class model
/////////////////////////////////////////////////////////////////////////
class model {
	public:
		typedef boost::shared_ptr<model> ptr;

	public:
		virtual ~model() {}
		long int id();
		virtual std::string json() = 0;
		static std::list<ptr> fetch_all(const std::string& type);
		static model::ptr factory(const std::string& type, const db::Result& vals);
		bool save();
		bool delete_row();

	protected:
		model();
		model(long int i);
		model(const db::Result& vals);
		virtual const char *table_name() const { return _nothing_; }
		virtual void _set(const std::string& name, const std::string& value) {
			_values[name] = value;
		}
		virtual std::string& _get(const std::string& name) {
			return _values[name];
		}
		virtual const std::vector<std::string>& names() const = 0;
		virtual std::vector<std::string> values() const;

	private:
		void opendb();
		static const char _nothing_[];
		static db::wptr _wdb;

	protected:
		db::ptr _db;
		db::Result _values;
};

//////////////////////////////////////////////////////////////////////////
// class static_dhcp
/////////////////////////////////////////////////////////////////////////
class static_dhcp : public model {
	public:
		virtual ~static_dhcp() {}
		static std::list<model::ptr> all();
		static static_dhcp::ptr create(const db::Result& vals);

	protected:
		static_dhcp() : model() {}
		static_dhcp(long int i) : model(i) {}
		static_dhcp(const db::Result& vals) : model(vals) {}
		virtual const char *table_name() const { return _table_name; }
		virtual const std::vector<std::string>& names() const;

	private:
		static const char _table_name[];

	// variable access interface
	public:
		virtual std::string json();
		std::string& hostname() { return _get("hostname"); }
		void hostname(const std::string& value) { _set("hostname", value); }
		std::string& ip_addr() { return _get("ip_addr"); }
		void ip_addr(const std::string& value) { _set("ip_addr", value); }
		std::string& mac_addr() { return _get("mac_addr"); }
		void mac_addr(const std::string& value) { _set("mac_addr", value); }
};

//////////////////////////////////////////////////////////////////////////
// class deny_dhcp
/////////////////////////////////////////////////////////////////////////
class deny_dhcp : public model {
	public:
		typedef boost::shared_ptr<deny_dhcp> ptr;

	public:
		virtual ~deny_dhcp() {}
		static std::list<model::ptr> all();
		static deny_dhcp::ptr create(const db::Result& vals);

	protected:
		deny_dhcp() : model() {}
		deny_dhcp(long int i) : model(i) {}
		deny_dhcp(const db::Result& vals) : model(vals) {}
		virtual const char *table_name() const { return _table_name; }
		virtual const std::vector<std::string>& names() const;

	private:
		static const char _table_name[];

	// variable access interface
	public:
		virtual std::string json();
		std::string& hostname() { return _get("hostname"); }
		void hostname(const std::string& value) { _set("hostname", value); }
		std::string& mac_addr() { return _get("mac_addr"); }
		void mac_addr(const std::string& value) { _set("mac_addr", value); }
};

//////////////////////////////////////////////////////////////////////////
// class firewall_forward
/////////////////////////////////////////////////////////////////////////
class firewall_forward : public model {
	public:
		typedef boost::shared_ptr<firewall_forward> ptr;

	public:
		virtual ~firewall_forward() {}
		static std::list<model::ptr> all();
		static firewall_forward::ptr create(const db::Result& vals);

	protected:
		firewall_forward() : model() {}
		firewall_forward(long int i) : model(i) {}
		firewall_forward(const db::Result& vals) : model(vals) {}
		virtual const char *table_name() const { return _table_name; }
		virtual const std::vector<std::string>& names() const;

	private:
		static const char _table_name[];

	// variable access interface
	public:
		virtual std::string json();
		std::string& protocol() { return _get("protocol"); }
		void protocol(const std::string& value) { _set("protocol", value); }
		std::string& src_port() { return _get("src_port"); }
		void src_port(const std::string& value) { _set("src_port", value); }
		std::string& port_range() { return _get("port_range"); }
		void port_range(const std::string& value) { _set("port_range", value); }
		std::string& dst_addr() { return _get("dst_addr"); }
		void dst_addr(const std::string& value) { _set("dst_addr", value); }
		std::string& dst_port() { return _get("dst_port"); }
		void dst_port(const std::string& value) { _set("dst_port", value); }
};

//////////////////////////////////////////////////////////////////////////
// class variable
/////////////////////////////////////////////////////////////////////////
class variable : public model {
	public:
		virtual ~variable() {}
		static std::list<model::ptr> all(const std::string& form_name);

		static std::string get(const std::string& vg, const std::string& n);
		static void set(const std::string& vg, const std::string &n, const std::string &v);
		static void del(const std::string& vg, const std::string &n);

	protected:
		variable() : model() {}
		variable(const std::string& vg) : model() { vgroup(vg); }
		variable(const std::string& vg, const std::string& n);
		variable(db::Result& vals) : model(vals) {}
		virtual const char *table_name() const { return _table_name; }
		virtual const std::vector<std::string>& names() const;

	private:
		static const char _table_name[];

	// variable access interface
	public:
		virtual std::string json();
		std::string& vgroup() { return _get("vgroup"); }
		void vgroup(const std::string& value) { _set("vgroup", value); }
		std::string& name() { return _get("name"); }
		void name(const std::string& value) { _set("name", value); }
		std::string& value() { return _get("value"); }
		void value(const std::string& value) { _set("value", value); }
};

#endif
