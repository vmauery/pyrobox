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

using std::basic_string;

#define foreach_last(T, A, M) for (T::iterator M##_next=A.begin(),\
		T::iterator M = M##_next++; \
		M != A.end(); M##_last++, M++)
#define foreach(T, A, M) for (T::iterator M = A.begin(); \
		M != A.end(); M++)

class JSON_Request: public Fastcgipp::Request<char>
{
	std::stringstream& model_all(std::stringstream& ss, const std::string& type) {
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

	std::stringstream& db_entries(std::stringstream& ss) {
		here();
		std::vector<std::string> models = explode(",", session.post["entries"]);
		foreach(std::vector<std::string>, models, i) {
			model_all(ss, *i);
		}
		return ss;
	}

	std::stringstream& form_response(std::stringstream& ss) {
		here();
		// build the form
		std::string form_file_name = "js/json/forms/";
		std::string form_name = session.post["form"];
		form_file_name += form_name;
		std::ifstream form_file;
		form_file.open(form_file_name.c_str(), std::ios::binary);
		if (form_file) {
		here();
			ss << "form: ";
			int bytes_read;
			form_file.seekg (0, std::ios::end);
			bytes_read = form_file.tellg();
			form_file.seekg (0, std::ios::beg);

			char *buf = new char[bytes_read];
			form_file.read(buf, bytes_read);
			ss.write(buf, bytes_read);
			form_file.close();
			delete[] buf;
			ss << ",";
			//std::list<form_values> = form_values::fetch(form_name);
			// verify input data
			// send the form (with return code/error info)
			ss << "form_values: {}";
			ss << ",\n";
		} else {
		here();
			ss << "form: {},\n";
		}
		return ss;
	}

	bool json_response() {
		here();
		std::stringstream ss;
		std::string response;
		ss << "{\n";
		// handle requests
			for(Fastcgipp::strmap::iterator it=session.post.begin(); it!=session.post.end(); ++it)
			{
				info(it->first << ": " << it->second);
			}
		if (session.post.find("form") != session.post.end()) {
			form_response(ss);
		}
		if (session.post.find("entries") != session.post.end()) {
			db_entries(ss);
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

	bool response()
	{
		/*
		if (session.server_port == 80) {
			// redirect to ssl
			string url = string("https://") + session.host + session.script_name + "/?" + session.query_string;
			out << "Location: " << url << "\r\n\r\n";
			return true;
		}
		*/
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

		// must be terminated with \r\n\r\n. NOT just \n\n.
		out << "Content-Type: text/html\r\n\r\n";

		// Now it's all stuff you should be familiar with
		out << "<html><head><meta http-equiv='Content-Type' content='text/html' />";
		out << "<title>Test page</title></head><body>";

		// This session data structure is defined in http.hpp
		out << "<h1>Session Parameters</h1>";
		for (Fastcgipp::strmap::iterator h=session.headers.begin(); h!=session.headers.end(); h++) {
			out << "<b>" << h->first << ":</b> " << h->second << "<br/>" << std::endl;
		}

		bool multipart = false;
		basic_string<wchar_type> enc;
		basic_string<wchar_type> file_input;
		if (session.get.find("multipart") != session.get.end()) {
			here();
			multipart = true;
			enc = " enctype='multipart/form-data'";
			file_input = "<div class=\"field-item\">\n<span class=\"label\"><label for=\"id_config_file\">Configuration file</label><div class=\"description\">Upload a saved configuration file.</div></span><span class=\"edit\"><input type=\"file\" name=\"config_file\" id=\"id_config_file\" /></span><span class=\"error\"></span>\n</div>\n";
		}
		out << "<form method='post'" << enc << ">" << std::endl
		    << "<div id=\"network-wan\" class=\"settings\">" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_wan_address_method\">WAN address method</label><div class=\"description\">Choose the address method provided by your ISP.  Most common would be DHCP and then PPPo_e.  Very few ISPs use static addresses.</div></span>" << std::endl
			<< "" << std::endl
			<< "	<span class=\"edit\"><select name=\"wan_address_method\" id=\"id_wan_address_method\">" << std::endl
			<< "<option value=\"dhcp\">Dynamic IP Address</option>" << std::endl
			<< "<option value=\"static\">Static IP Address</option>" << std::endl
			<< "<option value=\"pppoe\" selected=\"selected\">PPPo_e</option>" << std::endl
			<< "</select></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_username\">Username</label><div class=\"description\">Some ISPs require a username and password when connecting to their PPPo_e service.  If you were supplied with a username and password, enter them here.</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_username\" value=\"mauery\" id=\"id_pppoe_username\" /></span><span class=\"error\"></span>" << std::endl
			<< "" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_password\">Password</label><div class=\"description\">If this field is blank, that means no password is set.  If it is filled with '*', that means there is a password set. Be sure to enter the same password in both password fields.</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"password\" name=\"pppoe_password\" value=\"************\" id=\"id_pppoe_password\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_confirm_password\">Confirm password</label></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"password\" name=\"pppoe_confirm_password\" value=\"************\" id=\"id_pppoe_confirm_password\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_service_name\">Service name</label><div class=\"description\">Some ISPs require you to enter a service name.  If you were not suplied with one, leave this field blank.</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_service_name\" value=\"verizon\" id=\"id_pppoe_service_name\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_ip\">Static IP address</label><div class=\"description\">If you were assigned a static IP address for your connection,  you may enter it here. (This is not common.)</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_ip\" value=\"12.3.3.4\" id=\"id_pppoe_ip\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_mac_addr\">Custom MAC address</label><div class=\"description\">In some cases it is desirable or necessary to change the MAC address of the external network interface.  For most users, simply leave this field blank to use the default hardware MAC address.  MAC address format is '00:12:34:56:ac:df' or '00-12-34-56-ac-df'.</div></span>" << std::endl
			<< "" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_mac_addr\" value=\"00:33:32:af:de:46\" id=\"id_pppoe_mac_addr\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_dns_1\">Primary DNS server</label><div class=\"description\">Enter the IP address of the nameservers as provided by your ISP.  If you are running the DNS service on the local machine, this field will be ignored.</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_dns_1\" value=\"12.3.4.3\" id=\"id_pppoe_dns_1\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_dns_2\">Secondary DNS server</label></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_dns_2\" value=\"12.3.3.3\" id=\"id_pppoe_dns_2\" /></span><span class=\"error\"></span>" << std::endl
			<< "" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_mtu\">Network MTU</label><div class=\"description\">Unless you know what this is or are otherwise instructed to change it, leave it at the default value.</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"text\" name=\"pppoe_mtu\" value=\"1492\" id=\"id_pppoe_mtu\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< "<div class=\"field-item\">" << std::endl
			<< "	<span class=\"label\"><label for=\"id_pppoe_auto_reconnect\">Auto reconnect</label><div class=\"description\">Automatically re-establish the PPPo_e connection if it is reset for any reason.  This is recommended.</div></span>" << std::endl
			<< "	<span class=\"edit\"><input type=\"checkbox\" name=\"pppoe_auto_reconnect\" id=\"id_pppoe_auto_reconnect\" /></span><span class=\"error\"></span>" << std::endl
			<< "</div>" << std::endl
			<< file_input << std::endl
			<< "<div class=\"submit\"><input type='submit' value='Save'/></div>" << std::endl
			<< "</div>" << std::endl
			<< "</form>" << std::endl;


		out << "<h2>request data</h2><br>";
		for(Fastcgipp::strmap::iterator it=session.get.begin(); it!=session.get.end(); ++it) {
			out << it->first << ": " << it->second << "<br>";
			if (it->first == "multipart") here();
		}


		//Fastcgipp::Http::Post is defined in http.hpp
		out << "<h1>Post Data</h1>";
		if(session.post.size()) {
			for(Fastcgipp::strmap::iterator it=session.post.begin(); it!=session.post.end(); ++it)
			{
				out << it->first << ": " << it->second << "<br/>";
			}
		} else {
			out << "<p>No post data</p>";
			out << "<br><br>" << std::endl;
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
			out << "<br><br>" << std::endl;
		}

		char ** e = environ;
		while (*e) {
			out << *e << "<br>" << std::endl;
			e++;
		}

		out << "</body></html>";

		// Always return true if you are done. This will let apache know we are done
		// and the manager will destroy the request and free it's resources.
		// Return false if you are not finished but want to relinquish control and
		// allow other requests to operate. You might do this after an SQL query
		// while waiting for a reply. Passing messages to requests through the
		// manager is possible but beyond the scope of this example.
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
	Fastcgipp::logger::get()->add_target("/tmp/fcgi.log", Fastcgipp::LL_Info);

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
				perror("socket create");
				return 1;
			}

			int ret = 1;
			if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&ret, sizeof(ret))) < 0) {
				perror("setsockopt");
			}
			// make the connection
			if (bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
				perror("socket bind");
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
				perror("socket create");
				return 1;
			}

			// make the connection
			if (bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
				perror("socket bind");
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
}
