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
#include <db.hpp>
#include <model.hpp>

using std::basic_string;

/*
 element types
 fieldset
 textbox
 textarea
 checkbox
 radio
 select/multiselect
 combobox

{ type: "form", name: "dhcp-settings-form",
	// id: "defaults to name", class: "form ajax json", method: "post", action: "gobabygogo.cgi",
	elements: [
		{ type: "radio", name: "dhcp_enabled", label: "DHCP server", 
		  description: "Enable the DHCP server.",
		  default_value: "off", options:
			[
				{ label: "Enabled", value: "on" },
				{ label: "Disabled", value: "off" },
			],
		},
		{ type: "radio", name: "dhcp_dynamic_enabled", label: "Offer dynamic IP addresses", 
		  description: "Allow dynamic IP addresses to be handed out in addition to any static entries listed below.",
		  default_value: "off", options:
			[
				{ label: "Enabled", value: "on" },
				{ label: "Disabled", value: "off" },
			],
		},
		{ type: "textbox", name: "dhcp_start_ip", label: "Dynamic DHCP beginning address", 
		  description: "The DHCP server can give out IP addresses to other than the static entries below.  Enter the range of IP addresses to hand out dynamically here.  Make sure that this range does not overlap with the static DHCP entries.",
		  default_value: "10.0.0.100",
		},
		{ type: "textbox", name: "dhcp_end_ip", label: "Dynamic DHCP end address", 
		  default_value: "10.0.0.200",
		},
		{ type: "select", name: "dhcp_lease_time", label: "Lease length", 
		  description: "If you have a lot of DHCP users, you may want to keep this time short or the server may run out of IP addresses.  If you are only using static DHCP, then longer times means the clients won't ask the server for addresses as often.",
		  default_value: "86400", options:
			[
				{ label: "20 minutes", value: 1200 },
				{ label: "30 minutes", value: 1800 },
				{ label: "1 hour", value: 3600 },
				{ label: "3 hours", value: 10800 },
				{ label: "6 hours", value: 21600 },
				{ label: "12 hours", value: 43200},
				{ label: "1 day", value: 86400 },
				{ label: "2 days", value: 172800 },
				{ label: "3 days", value: 259200 },
				{ label: "4 days", value: 345600 },
				{ label: "5 days", value: 432000 },
				{ label: "6 days", value: 518400 },
				{ label: "1 week", value: 604800 },
			],
		},
	]
}

*/

class JSON_Request: public Fastcgipp::Request<wchar_type>
{
	bool json_response() {
		std::basic_string<wchar_type> response;
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
		out << "Content-Type: application/json\r\n"
			<< "Expires: Wed, 14 Jan 2000 05:45:48 GMT\r\n"
		    << "Content-Length: " << response.length() << "\r\n\r\n"
			<< response;

		return true;
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
		if (session.headers[_S("REQUEST_METHOD")] == _S("POST") && 
			session.headers[_S("HTTP_ACCEPT")].find(_S("application/json")) != std::basic_string<wchar_type>::npos) {
			return json_response();
		}

		std::map<basic_string<wchar_type>,basic_string<wchar_type> >::const_iterator filename;
		if ((filename = session.get.find(_S("file"))) != session.get.end()) {
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
		for (std::map<std::basic_string<wchar_type>,std::basic_string<wchar_type> >::iterator h=session.headers.begin(); h!=session.headers.end(); h++) {
			out << "<b>" << h->first << ":</b> " << h->second << "<br/>" << std::endl;
		}

		bool multipart = false;
		basic_string<wchar_type> enc;
		if (session.get.find(_S("multipart")) != session.get.end()) {
			here();
			multipart = true;
			enc = _S(" enctype='multipart/form-data'");
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
			<< "" << std::endl
			<< "<div class=\"submit\"><input type='submit' value='Save'/></div>" << std::endl
			<< "</div>" << std::endl
			<< "</form>" << std::endl;


		out << "<h2>request data</h2><br>";
		for(std::map<basic_string<wchar_type>,basic_string<wchar_type> >::iterator it=session.get.begin(); it!=session.get.end(); ++it) {
			out << it->first << ": " << it->second << "<br>";
			if (it->first == _S("multipart")) here();
		}


		//Fastcgipp::Http::Post is defined in http.hpp
		out << "<h1>Post Data</h1>";
		if(session.post.size())
			for(Fastcgipp::Http::Session<wchar_type>::Posts::iterator it=session.post.begin(); it!=session.post.end(); ++it)
			{
				out << "<h2>" << it->first << "</h2>";
				if(it->second.type==Fastcgipp::Http::Post<wchar_type>::form)
				{
					out << "<p><b>Type:</b> form data<br />";
					out << "<b>Value:</b> " << it->second.value << "</p>";
				}
				
				else
				{
					out << "<p><b>Type:</b> file<br />";
					// When the post type is a file, the filename is stored in Post::value;
					out << "<b>Filename:</b> " << it->second.value << "<br />";
					out << "<b>Size:</b> " << it->second.size << "<br />";
					out << "<b>Data:</b></p><pre>";
					// We will use dump to send the raw data directly to the client
					out.dump(it->second.data.get(), it->second.size);
					out << "</pre>";
				}
			}
		else
			out << "<p>No post data</p>";
		out << "<br><br>" << std::endl;

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
			msg("starting with stdin as socket");
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
		msg(e.what());
	}
}
