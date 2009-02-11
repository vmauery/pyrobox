#ifndef _FORM_HPP_
#define _FORM_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <sstream>
#include <map>

class form {
	public:
		typedef boost::shared_ptr<form> ptr;
		struct does_not_exist {
			does_not_exist(const std::string& name) : _name(name) {}
			std::string _name;
		};

	public:
		static form::ptr create(const std::string& name);
		virtual ~form() {}

		std::string submit(const std::map<std::string,std::string>& post);
		std::string render();

	protected:
		form(const std::string& name);
		void render_values(std::stringstream& ss);

		std::map<std::string,std::string> validate(const std::map<std::string,std::string>& values);
		// validator
		bool valid_string(const std::string& v);
		bool valid_int(const std::string& v);
		bool valid_float(const std::string& v);
		bool valid_port(const std::string& v);
		bool valid_ipv4(const std::string& v);
		bool valid_ipv6(const std::string& v);
		bool valid_mac(const std::string& v);
		bool valid_hostname(const std::string& v);
		bool valid_list(const std::string& v);
	

	protected:
		std::string _name;
		boost::scoped_ptr<char> _data;
		size_t _data_len;
};

#endif
