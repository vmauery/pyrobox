#ifndef _FORM_HPP_
#define _FORM_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <sstream>
#include <map>
#include <vector>

typedef std::map<std::string,std::string> strmap;

class form_set;
class form_base {
	public:
		typedef boost::shared_ptr<form_base> ptr;
		typedef boost::weak_ptr<form_base> wptr;
		typedef enum {
			t_form_element = 0,
			t_form_set,
			t_field_set,
			t_form,
			t_button,
			t_submit,
			t_textarea,
			t_textbox,
			t_password,
			t_multiple_choice,
			t_select,
			t_combobox,
			t_radios,
			t_checkbox,
			t_checkboxes,
			t_hidden,
			t_file,
			element_count
		} form_element_type;
		class invalid_type {
			public:
				invalid_type(const std::string& type) : _type(type) {}
				std::string _type;
		};

	protected:
		form_element_type _type;
		strmap _attrs;
		boost::weak_ptr<form_set> _parent;

		form_base(form_element_type type, const strmap& attrs) : _type(type), _attrs(attrs) {
			if (_attrs.find("name") == _attrs.end()) {
				_attrs["name"] == "";
			}
		}

		std::string render_attrs() const;
		form_element_type type() const { return _type; }
		const char* type_name() const;

	public:
		virtual ~form_base() {}
		virtual const std::string& name() const { return _attrs.find("name")->second; }
		virtual void attr(const std::string& n, const std::string& v) {
			_attrs[n] = v;
		}
		virtual void parent(boost::shared_ptr<form_set> p) {
			_parent = boost::weak_ptr<form_set>(p);
		}
		virtual boost::shared_ptr<form_set> parent() {
			return _parent.lock();
		}
		virtual bool valid() const = 0;
		virtual std::string render() const = 0;
		static ptr factory(const std::string& type, const strmap& attrs=strmap(), const strmap& options=strmap());
};

class form_element : public form_base {
	public:
		typedef boost::shared_ptr<form_element> ptr;
		typedef boost::weak_ptr<form_element> wptr;

	protected:
		form_element(form_element_type type, const strmap& attrs) : form_base(type, attrs) {}

	public:
		virtual ~form_element() {}

		virtual bool valid() const = 0;
		virtual std::string render() const;
};

class form_set : public form_base {
	public:
		typedef boost::shared_ptr<form_set> ptr;
		typedef boost::weak_ptr<form_set> wptr;
		friend class field_set;
		friend class form;
		friend class button;
		friend class submit;
		friend class textarea;
		friend class textbox;
		friend class password;
		friend class select;
		friend class combobox;
		friend class radios;
		friend class checkbox;
		friend class checkboxes;
		friend class hidden;
		friend class file;

	protected:
		std::vector<form_base::ptr> _children;
		form_set(form_element_type type, const strmap& attrs) : form_base(type, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~form_set() {}
		virtual std::string render() const;
		virtual bool valid() const;
		virtual void add(form_base::ptr child) {
			_children.push_back(child);
		}
};

class field_set : public form_set {
	public:
		typedef boost::shared_ptr<field_set> ptr;

	protected:
		field_set(const strmap& attrs) : form_set(t_field_set, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~field_set() {}
};

class button : public form_element {
	public:
		typedef boost::shared_ptr<button> ptr;

	protected:
		button(const strmap& attrs) : form_element(t_button, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~button() {}
		virtual bool valid() const;
};

class submit : public form_element {
	public:
		typedef boost::shared_ptr<submit> ptr;

	protected:
		submit(const strmap& attrs) : form_element(t_submit, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~submit() {}
		virtual bool valid() const;
};

class textarea : public form_element {
	public:
		typedef boost::shared_ptr<textarea> ptr;

	protected:
		textarea(const strmap& attrs) : form_element(t_textarea, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~textarea() {}
		virtual bool valid() const;
};

class textbox : public form_element {
	public:
		typedef boost::shared_ptr<textbox> ptr;

	protected:
		textbox(const strmap& attrs) : form_element(t_textbox, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~textbox() {}
		virtual bool valid() const;
};

class password : public form_element {
	public:
		typedef boost::shared_ptr<password> ptr;

	protected:
		password(const strmap& attrs) : form_element(t_password, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~password() {}
		virtual bool valid() const;
};

class multiple_choice : public form_element {
	public:
		typedef boost::shared_ptr<multiple_choice> ptr;

	protected:
		multiple_choice(form_element_type type, const strmap& attrs, const strmap& options) : form_element(type, attrs), _options(options) {}

		strmap _options;

	public:
		virtual ~multiple_choice() {}
		virtual std::string render() const;
		virtual bool valid() const;
		virtual void option(const std::string& n, const std::string& v) {
			_options[n] = v;
		}
};

class select : public multiple_choice {
	public:
		typedef boost::shared_ptr<select> ptr;

	protected:
		select(const strmap& attrs, const strmap& options) : multiple_choice(t_select, attrs, options) {}
		bool multi_;

	public:
		static ptr create(const strmap& attrs=strmap(), const strmap& options=strmap());
		virtual ~select() {}
};

class radios : public multiple_choice {
	public:
		typedef boost::shared_ptr<radios> ptr;

	protected:
		radios(const strmap& attrs, const strmap& options) : multiple_choice(t_radios, attrs, options) {}

	public:
		static ptr create(const strmap& attrs=strmap(), const strmap& options=strmap());
		virtual ~radios() {}
};

class combobox : public multiple_choice {
	public:
		typedef boost::shared_ptr<combobox> ptr;

	protected:
		combobox(const strmap& attrs, const strmap& options) : multiple_choice(t_combobox, attrs, options) {}

	public:
		static ptr create(const strmap& attrs=strmap(), const strmap& options=strmap());
		virtual ~combobox() {}
};

class checkbox : public form_element {
	public:
		typedef boost::shared_ptr<checkbox> ptr;

	protected:
		checkbox(const strmap& attrs) : form_element(t_checkbox, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~checkbox() {}
		virtual bool valid() const;
};

class checkboxes : public multiple_choice {
	public:
		typedef boost::shared_ptr<checkboxes> ptr;

	protected:
		checkboxes(const strmap& attrs, const strmap& options) : multiple_choice(t_checkboxes, attrs, options) {}

	public:
		static ptr create(const strmap& attrs=strmap(), const strmap& options=strmap());
		virtual ~checkboxes() {}
};

class file : public form_element {
	public:
		typedef boost::shared_ptr<file> ptr;

	protected:
		file(const strmap& attrs) : form_element(t_file, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~file() {}
		virtual bool valid() const;
};

class hidden : public form_element {
	public:
		typedef boost::shared_ptr<hidden> ptr;

	protected:
		hidden(const strmap& attrs) : form_element(t_hidden, attrs) {}

	public:
		static ptr create(const strmap& attrs=strmap());
		virtual ~hidden() {}
		virtual bool valid() const;
};


class form : public form_set {
	public:
		typedef boost::shared_ptr<form> ptr;
		typedef boost::weak_ptr<form> wptr;
		struct does_not_exist {
			does_not_exist(const std::string& name) : _name(name) {}
			std::string _name;
		};
		struct parse_error {
			parse_error(const std::string& msg, const std::string& line, int lineno) : _msg(msg), _line(line), _lineno(lineno) {}
			std::string _msg;
			std::string _line;
			int _lineno;
		};

	public:
		// unlike the other create methods, this method parses
		// a file to create the class and all its children
		static ptr create(const std::string& name);
		static ptr create(const strmap& attrs=strmap());

		virtual ~form() {}

		std::string submit(const strmap& post) const;
		std::string render_values() const;

	protected:
		form(const strmap& attrs) : form_set(t_form, attrs) {}

		strmap validate(const strmap& values) const;

};

#endif
