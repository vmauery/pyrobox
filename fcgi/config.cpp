/***************************************************************************
* Copyright (C) 2007 Eddie                                                 *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/

// for sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>
#define UNIX_PATH_MAX    108
// for stat
#include <sys/stat.h>
#include <unistd.h>
// for strerror_r and errno
#include <string.h>
#include <errno.h>
// for ostringstream
#include <iostream>
#include <fstream>
#include <sstream>
// catch unix signals
#include <signal.h>
#include <iconv.h>

#include <request.hpp>
#include <manager.hpp>
#include <util.hpp>
#include <logging.hpp>
#include <db.hpp>
#include <model.hpp>
#include <debug.hpp>
#include <form.hpp>

using std::basic_string;

#define foreach_last(T, A, M) for (T::iterator M##_next=A.begin(),\
		T::iterator M = M##_next++; \
		M != A.end(); M##_last++, M++)
#define foreach(T, A, M) for (T::iterator M = A.begin(); \
		M != A.end(); M++)

class JSON_Request: public Fastcgipp::Request<char>
{
	std::stringstream& db_entries(std::stringstream& ss, const std::string& type) {
		here();
		std::list<model::ptr>::iterator rowiter;
		std::list<model::ptr> models = model::fetch_all(type);
		ss << type << ": [\n";
		for (rowiter=models.begin(); rowiter!=models.end(); ) {
			ss << (*rowiter)->json();
			if (++rowiter != models.end()) {
				ss << ",\n";
			}
		}
		ss << "\n],\n";
		return ss;
	}

	std::stringstream& form_response(std::stringstream& ss, const std::string& form_name) {
		here();
		return ss;
	}

	bool json_response() {
		here();
		std::stringstream ss;
		std::string response;
		ss << "{\n";
		// handle requests
		Fastcgipp::strmap::iterator it;
		for(it=session.post.begin(); it!=session.post.end(); ++it)
		{
			info(it->first << ": " << it->second);
		}
		if ((it = session.get.find("form_submit")) != session.get.end()) {
			try {
				form::ptr f = form::create(it->second);
				ss << f->submit(session.post);
			} catch (form::does_not_exist e) {
				// FIXME: form error path?
			}
		} else {
			foreach(Fastcgipp::strmap, session.post, post) {
				if (post->second == "form") {
					try {
						form::ptr f = form::create(post->first);
						ss << f->render();
					} catch (form::does_not_exist e) {
						ss << post->first << ": {form: {elements:[]}, values: {}, },\n";
					}
				} else if (post->second == "records") {
					db_entries(ss, post->first);
				}
			}
		}
		ss << "}";
		response = ss.str();
		out << "Content-Type: application/json\r\n"
			<< "Expires: Wed, 31 Dec 1969, 23:59:59 GMT\r\n"
		    << "Content-Length: " << response.length() << "\r\n\r\n"
			<< response;

		info(std::endl << "Content-Type: application/json\r\n"
			<< "Expires: Wed, 31 Dec 1969, 23:59:59 GMT\r\n"
		    << "Content-Length: " << response.length() << "\r\n\r\n"
			<< response);


		return true;
	}

	bool debug()
	{
		// This session data structure is defined in http.hpp
		info("**** Session Parameters ****");
		for (Fastcgipp::strmap::iterator h=session.headers.begin(); h!=session.headers.end(); h++) {
			info(h->first << ": " << h->second);
		}

		info("*** Request data ***");
		for(Fastcgipp::strmap::iterator it=session.get.begin(); it!=session.get.end(); ++it) {
			info(it->first << ": " << it->second);
		}


		//Fastcgipp::Http::Post is defined in http.hpp
		info("*** Post Data ***");
		if(session.post.size()) {
			for(Fastcgipp::strmap::iterator it=session.post.begin(); it!=session.post.end(); ++it)
			{
				info(it->first << ": " << it->second);
			}
		} else {
			info("No post data");
		}
		info("*** Files ***");
		if(session.files.size()) {
			for(Fastcgipp::filemap::iterator it=session.files.begin(); it!=session.files.end(); ++it) {
				// When the post type is a file, the filename is stored in Post::value;
				info(std::endl << "Filename: " << it->second->name() << std::endl
				    << "path: " << it->second->path() << std::endl
				    << "mime: " << it->second->mime() << std::endl
				    << "Size: " << it->second->size() << std::endl
				    << "contents:" << std::endl);
				log_dump(it->second->data().get(), it->second->size());
			}
		} else {
			info("No uploaded files");
		}

		char ** e = environ;
		if (*e) {
			info("Environment");
			while (*e) {
				info(*e);
				e++;
			}
		}
	}

	bool test_page() {
		// must be terminated with \r\n\r\n. NOT just \n\n.
		out << "Content-Type: text/html\r\n\r\n";

		// Now it's all stuff you should be familiar with
		out << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\"><head><style type=\"text/css\" media=\"all\">@import \"/theme/flames/style.css\";</style><meta http-equiv='Content-Type' content='text/html' />" << std::endl
			<< "<title>Test page</title></head><body class=\"sidebar-left\">" << std::endl
			<< "<div id=\"header-region\" class=\"clear-block\"></div>" << std::endl
			<< "<div id=\"wrapper\">" << std::endl
			<< "\t<div id=\"container\" class=\"clear-block\">" << std::endl
			<< "\t\t<div id=\"header\">" << std::endl
			<< "\t\t\t<div id=\"logo-floater\"><h1><a href=\"/\"><img src=\"/theme/flames/images/logo.png\" alt=\"\" title=\"\"  /><span>.-~::PyroBox::~-.</span></a></h1></div>" << std::endl
			<< "\t\t\t<ul class=\"links primary-links\">" << std::endl
			<< "\t\t\t\t<li><a href=\"/settings\" class=\"active\">Settings</a></li>" << std::endl
			<< "\t\t\t\t<li><a href=\"/firewall\" class=\"\">Firewall</a></li>" << std::endl
			<< "\t\t\t\t<li><a href=\"/\" class=\"\">Information</a></li>" << std::endl
			<< "\t\t\t\t<li><a href=\"/network\" class=\"\">Network</a></li>" << std::endl
			<< "\t\t\t</ul>" << std::endl
			<< "\t\t\t<ul class=\"links secondary-links\">" << std::endl
			<< "\t\t\t\t<li><a href=\"/settings/dhcp\" class=\"active\">DHCP</a></li>" << std::endl
			<< "\t\t\t\t<li><a href=\"/settings/dns\" class=\"\">DNS</a></li>" << std::endl
			<< "\t\t\t\t<li><a href=\"/settings/captive\" class=\"\">Captive Portal</a></li>" << std::endl
			<< "\t\t\t</ul>" << std::endl
			<< "\t\t</div><!-- /header -->" << std::endl
			<< "\t\t<div id=\"sidebar-left\" class=\"sidebar\">" << std::endl
			<< "\t\t</div>" << std::endl
			<< "\t\t<div id=\"center\"><div id=\"squeeze\"><div class=\"right-corner\"><div class=\"left-corner\">" << std::endl
			<< "<div class=\"breadcrumb\"><a href=\"/\">Home</a> &raquo; <a href=\"/settings\">Settings</a> &raquo; <a href=\"/settings/dhcp\">DHCP</a></div>" << std::endl
			<< "\t\t<div id=\"blocks-pre\"></div>" << std::endl
			<< "<div id=\"content\">" << std::endl;


		bool multipart = false;
		basic_string<wchar_type> enc;
		basic_string<wchar_type> file_input;
		if (session.get.find("multipart") != session.get.end()) {
			here();
			multipart = true;
			enc = " enctype='multipart/form-data'";
			file_input = "<div class=\"field-item\">\n<div class=\"label\"><label for=\"id_config_file\">Configuration file</label><div class=\"description\">Upload a saved configuration file.</div></div><span class=\"edit\"><input type=\"file\" name=\"config_file\" id=\"id_config_file\" /></span><span class=\"error\"></span>\n</div>\n";
		}
		out << "<form method='post' action='' " << enc << ">" << std::endl
		    << "<div id=\"network-wan\" class=\"settings\">" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_wan_address_method\">WAN address method</label><div class=\"description\">Choose the address method provided by your ISP.  Most common would be DHCP and then PPPo_e.  Very few ISPs use static addresses.</div></div>" << std::endl
			<< "" << std::endl
			<< "	<span class=\"edit\"><select name=\"wan_address_method\" id=\"id_wan_address_method\">" << std::endl
			<< "<option value=\"dhcp\">Dynamic IP Address</option>" << std::endl
			<< "<option value=\"static\">Static IP Address</option>" << std::endl
			<< "<option value=\"pppoe\" selected=\"selected\">PPPo_e</option>" << std::endl
			<< "</select></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_username\">Username</label><div class=\"description\">Some ISPs require a username and password when connecting to their PPPo_e service.  If you were supplied with a username and password, enter them here.</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_username\" value=\"mauery\" id=\"id_pppoe_username\" /></span><span class=\"error\"></span>" << std::endl
			<< "" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_password\">Password</label><div class=\"description\">If this field is blank, that means no password is set.  If it is filled with '*', that means there is a password set. Be sure to enter the same password in both password fields.</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"password\" name=\"pppoe_password\" value=\"************\" id=\"id_pppoe_password\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_confirm_password\">Confirm password</label></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"password\" name=\"pppoe_confirm_password\" value=\"************\" id=\"id_pppoe_confirm_password\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_service_name\">Service name</label><div class=\"description\">Some ISPs require you to enter a service name.  If you were not suplied with one, leave this field blank.</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_service_name\" value=\"verizon\" id=\"id_pppoe_service_name\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_ip\">Static IP address</label><div class=\"description\">If you were assigned a static IP address for your connection,  you may enter it here. (This is not common.)</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_ip\" value=\"12.3.3.4\" id=\"id_pppoe_ip\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_mac_addr\">Custom MAC address</label><div class=\"description\">In some cases it is desirable or necessary to change the MAC address of the external network interface.  For most users, simply leave this field blank to use the default hardware MAC address.  MAC address format is '00:12:34:56:ac:df' or '00-12-34-56-ac-df'.</div></div>" << std::endl
			<< "" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_mac_addr\" value=\"00:33:32:af:de:46\" id=\"id_pppoe_mac_addr\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_dns_1\">Primary DNS server</label><div class=\"description\">Enter the IP address of the nameservers as provided by your ISP.  If you are running the DNS service on the local machine, this field will be ignored.</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_dns_1\" value=\"12.3.4.3\" id=\"id_pppoe_dns_1\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_dns_2\">Secondary DNS server</label></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_dns_2\" value=\"12.3.3.3\" id=\"id_pppoe_dns_2\" /></span><span class=\"error\"></span>" << std::endl
			<< "" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_mtu\">Network MTU</label><div class=\"description\">Unless you know what this is or are otherwise instructed to change it, leave it at the default value.</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_mtu\" value=\"1492\" id=\"id_pppoe_mtu\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<div class=\"label\"><label for=\"id_pppoe_auto_reconnect\">Auto reconnect</label><div class=\"description\">Automatically re-establish the PPPo_e connection if it is reset for any reason.  This is recommended.</div></div>" << std::endl
			<< "	<span class=\"edit\"><input type=\"checkbox\" name=\"pppoe_auto_reconnect\" id=\"id_pppoe_auto_reconnect\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< file_input << std::endl
			<< "<div class=\"submit\"><input type='submit' value='Save'/></div>" << std::endl
			<< "</div>" << std::endl
			<< "</form>" << std::endl;


		// This session data structure is defined in http.hpp
		out << "<h1>Session Parameters</h1>";
		for (Fastcgipp::strmap::iterator h=session.headers.begin(); h!=session.headers.end(); h++) {
			out << "<b>" << h->first << ":</b> " << html_entities(h->second) << "<br/>" << std::endl;
		}

		out << "<h1>request data</h1>";
		for(Fastcgipp::strmap::iterator it=session.get.begin(); it!=session.get.end(); ++it) {
			out << it->first << ": " << html_entities(it->second) << "<br/>";
		}


		//Fastcgipp::Http::Post is defined in http.hpp
		out << "<h1>Post Data</h1>";
		if(session.post.size()) {
			for(Fastcgipp::strmap::iterator it=session.post.begin(); it!=session.post.end(); ++it)
			{
				out << it->first << ": " << html_entities(it->second) << "<br/>";
			}
		} else {
			out << "<p>No post data</p>";
			out << "<br/>" << std::endl;
		}
		out << "<h1>Files</h1>";
		if(session.files.size()) {
			for(Fastcgipp::filemap::iterator it=session.files.begin(); it!=session.files.end(); ++it) {
				// When the post type is a file, the filename is stored in Post::value;
				out << "<p><b>Filename:</b> " << it->second->name() << "<br />";
				out << "<p>path:</b> " << it->second->path() << "<br />";
				out << "<p>mime:</b> " << it->second->mime() << "<br />";
				out << "<b>Size:</b> " << it->second->size() << "<br />";
				out << "contents:</p><pre>";
				out.dump(it->second->data().get(), it->second->size());
				out << "</pre>";
			}
		} else {
			out << "<p>No uploaded files</p>";
			out << "<br/><br/>" << std::endl;
		}

		char ** e = environ;
		if (*e)
			out << "<h1>Environ</h1>";
		while (*e) {
			out << *e << "<br/>" << std::endl;
			e++;
		}

		out << "</div></div></div></div></div></div></div></body></html>";
		return true;
	}

	bool response()
	{
		if (session.server_port == 80) {
			// redirect to ssl
			std::string url = std::string("https://") + session.host + session.script_name + "/?" + session.query_string;
			out << "Status: 301 Moved Permanently\r\n";
			out << "Location: " << url << "\r\n\r\n";
			return true;
		}
		if (session.get.find("debug") != session.get.end()) {
			debug();
		}
		if (session.headers["REQUEST_METHOD"] == "POST" && 
			session.headers["HTTP_ACCEPT"].find("application/json") != std::string::npos) {
			return json_response();
		}

		here();
		Fastcgipp::strmap::const_iterator filename;
		if ((filename = session.get.find("file")) != session.get.end()) {
			if (sendfile(filename->second))
				return true;
		}
		
		
		if (session.get.find("test") != session.get.end()) {
			return test_page();
		}

		out << "Status: 404 Not Found\r\n";
		out << "Content-Type: text/html\r\n\r\n";
		out << "<html><head><title>Pyrobox FastCGI server / Page Not Found</title></head>" << std::endl;
		out << "<body>Could not find requested page<br/>" << std::endl;
		out << "try ?test&amp;multipart&amp;debug" << std::endl;
		out << "</body></html>" << std::endl;

		return true;
	}

	bool sendfile(const basic_string<wchar_type> &filename) {
#ifdef UNICODE_TEXT
		iconv_t ic = iconv_open("US-ASCII", "UNICODE");
		char outbuf[512];
		size_t outleft = sizeof(outbuf);
		char *inbuf = (char*)filename.c_str();
		size_t inleft = filename.length();
		iconv(ic, &inbuf, &inleft, (char**)&outbuf, &outleft);
#else
		const char *outbuf = filename.c_str();
#endif

		std::ifstream file;
		file.open(outbuf, std::ios::binary);
		if (file) {
			int bytes_read;
			file.seekg (0, std::ios::end);
			bytes_read = file.tellg();
			file.seekg (0, std::ios::beg);

			out << "Content-Type: text/html\r\n_content-Length: "
				<< bytes_read << "\r\n\r\n";

			char *buf = new char[bytes_read];
			file.read(buf, bytes_read);
			out.dump(buf, bytes_read);
			file.close();
			delete[] buf;
			return true;
		}
		return false;
	}

};

int prog_argc;
char **prog_argv;
char **prog_env;

void usage(char *prog) {
	std::cerr << "usage: " << prog << " [-h hostname] [-p port]" << std::endl;
	exit(0);
}

// The main function is easy to set up
int main(int argc, char **argv, char **env)
{
	int sock = 0;
	int port = 4545, address = 127 << 24 | 0 << 16 | 0 << 8 | 1 << 0;
	char *hostname = NULL, *unixpath = NULL;
	prog_argc = argc;
	prog_argv = argv;
	prog_env = env;
	std::string conf_file = "/etc/pyrobox.conf";

	read_config_file(conf_file);

	for (int i=1; i<argc; i++) {
		if (strncmp(argv[i], "-p", 2) == 0) {
			if (++i < argc) {
				port = atoi(argv[i]);
			} else {
				usage(argv[0]);
			}
		} else if (strncmp(argv[i], "-h", 2) == 0) {
			if (++i < argc) {
				hostname = argv[i];
			} else {
				usage(argv[0]);
			}
		} else if (strncmp(argv[i], "-u", 2) == 0) {
			if (++i < argc) {
				unixpath = argv[i];
			} else {
				usage(argv[0]);
			}
		}
	}

	Fastcgipp::logger::get()->add_target(get_conf("log_file").c_str(), Fastcgipp::LL_Info);

	struct sockaddr saddr;
	socklen_t len;
	int socket_type = -1;
	memset(&saddr, 0, sizeof(saddr));
	if (getpeername(0, &saddr, &len) == -1 && errno == ENOTCONN) {
		socket_type = 0;
	} else {
		if (unixpath) {
			socket_type = 2;
		} else {
			socket_type = 1;
		}
	}
	switch (socket_type) {
		case 0:
			info("starting with stdin as socket");
			break;
		case 1:
		{
			struct sockaddr_in sockaddr;
			memset(&sockaddr, 0, sizeof(sockaddr));
			// set up sockaddr
			sockaddr.sin_port = htons((unsigned short)port);
			sockaddr.sin_family = AF_INET;
			if (hostname) {
				struct hostent *host = gethostbyname(hostname);
				memcpy(&sockaddr.sin_addr, host->h_addr_list[0], host->h_length);   
			} else {
				sockaddr.sin_addr.s_addr = htonl(address);
			}

			// get a socket
			sock = socket(PF_INET, SOCK_STREAM, 0);
			if (sock < 0) {
				error("socket:" << strerror(errno));
				return 1;
			}

			int ret = 1;
			if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&ret, sizeof(ret))) < 0) {
				error("setsockopt:" << strerror(errno));
			}
			// make the connection
			if (bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
				error("bind:" << strerror(errno));
				return 1;
			}
			listen(sock, 0);
			break;
		}
		case 2:
		{
			struct sockaddr_un sockaddr;

			// set up sockaddr
			memset(&sockaddr, 0, sizeof(sockaddr));
			sockaddr.sun_family = AF_UNIX;
			strncpy(sockaddr.sun_path, unixpath, UNIX_PATH_MAX);

			// get a socket
			sock = socket(PF_UNIX, SOCK_STREAM, 0);
			if (sock < 0) {
				error("socket:" << strerror(errno));
				return 1;
			}

			// make the connection
			if (bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
				error("bind:" << strerror(errno));
				return 1;
			}
			listen(sock, 0);
			break;
		}
	}

	try
	{
		// First we make a Fastcgipp::Manager object, with our request handling class
		// as a template parameter.
		Fastcgipp::Manager<JSON_Request> fcgi(sock);
		// Now just call the object handler function. It will sleep quietly when there
		// are no requests and efficiently manage them when there are many.
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		// Catch any exception and put them in our errlog file.
		error(e.what());
	}

	debug_mem_fini();
	return 0;
}
