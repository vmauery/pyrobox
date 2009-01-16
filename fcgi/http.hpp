//! \file http.hpp Defines elements of the HTTP protocol
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


#ifndef HTTP_HPP
#define HTTP_HPP

#include <string>
#include <boost/shared_array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ostream>
#include <istream>
#include <cstring>
#include <sstream>
#include <map>

#include <exceptions.hpp>
#include <protocol.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! Defines classes and function relating to the http protocol
	namespace Http
	{
		//! Holds a piece of HTTP post data
		/*!
		 * This structure will hold one of two types of HTTP post data. It can
		 * either contain form data, in which case the data field is empty and
		 * the size is zero; or it can hold an uploaded file, in which case data
		 * contains a pointer to the file data, size contains it's size and value holds it's
		 * filename. The actual name associated with the piece of post data
		 * is omitted from the class so it can be linked in an associative
		 * container.
		 *
		 * @tparam char_t Type of character to use in the value string (char or wchar_t)
		 */
		template<class char_t> struct Post
		{
			//! Type of POST data piece
			enum Type { file, form } type;
			//! Value of POST data if type=form or the filename if type=file
			std::basic_string<char_t> value;
			//! Pointer to file data
			boost::shared_array<char> data;
			//! Size of data in bytes pointed to by data.
			size_t size;
		};

		//! Efficiently stores IPv4 addresses
		/*!
		 * This class stores IPv4 addresses as unsigned 32bit integers. It does this
		 * as opposed to storing the string itself to facilitate efficient logging and
		 * processing of the address. The class possesses full IO and comparison capabilities
		 * as well as allowing bitwise AND operations for netmask calculation.
		 */
		class Address
		{
		public:
			//! Retrieve the integer value of the IPv4 address
			/*!
			 * @return Unsigned 32bit integer representing the IPv4 address
			 */
			uint32_t get_int() const { return data; }
			//! Assign the IPv4 address from an integer
			/*!
			 * @param[in] data_ Unsigned 32bit integer representing the IPv4 address
			 */
			Address operator=(uint32_t data_) { data=data_; return *this; }
			Address operator=(Address address) { data=address.data; return *this; }
			Address(const Address& address): data(address.data) { }
			//! Construct the IPv4 address from an integer
			/*!
			 * @param[in] data_ Unsigned 32bit integer representing the IPv4 address
			 */
			Address(uint32_t data_): data(data_) { }
			//! Constructs from a value of 0.0.0.0 (0)
			Address(): data(0) { }
			//! Assign the IPv4 address from a string of characters
			/*!
			 * In order for this to work the string must represent an IPv4 address in
			 * textual decimal form and nothing else. Example: "127.0.0.1".
			 *
			 * @param[in] start First character of the string
			 * @param[in] end Last character of the string + 1
			 */
			void assign(const char* start, const char* end);
		private:
			friend inline bool operator==(Address x, Address y);
			friend inline bool operator>(Address x, Address y);
			friend inline bool operator<(Address x, Address y);
			friend inline bool operator<=(Address x, Address y);
			friend inline bool operator>=(Address x, Address y);
			friend inline Address operator&(Address x, Address y);
			template<class char_t, class Traits> friend std::basic_ostream<char_t, Traits>& operator<<(std::basic_ostream<char_t, Traits>& os, const Address& address);
			template<class char_t, class Traits> friend std::basic_istream<char_t, Traits>& operator>>(std::basic_istream<char_t, Traits>& is, Address& address);
			//! Data representation of the IPv4 address
			uint32_t data;
		};

		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator==(Address x, Address y) { return x.data==y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator>(Address x, Address y) { return x.data>y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator<(Address x, Address y) { return x.data<y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator<=(Address x, Address y) { return x.data<=y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator>=(Address x, Address y) { return x.data>=y.data; }
		//! Bitwise AND two Address values
		/*!
		 * The bitwise AND operation is passed on to the internal unsigned 32 bit integer
		 */
		inline Address operator&(Address x, Address y) { return Address(x.data&y.data); }

		//! Address stream insertion operation
		/*!
		 * This stream inserter obeys all stream manipulators regarding alignment, field width and numerical base.
		 */
		template<class char_t, class Traits> std::basic_ostream<char_t, Traits>& operator<<(std::basic_ostream<char_t, Traits>& os, const Address& address);
		//! Address stream extractor operation
		/*!
		 * In order for this to work the stream must be positioned on at the start of a standard decimal representation
		 * of a IPv4 address. Example: "127.0.0.1".
		 */
		template<class char_t, class Traits> std::basic_istream<char_t, Traits>& operator>>(std::basic_istream<char_t, Traits>& is, Address& address);

		//! Data structure of HTTP session data
		/*!
		 * This structure contains all HTTP session data for each individual request. The data is processed
		 * from Fast_cGI parameter records.
		 *
		 * @tparam char_t Character type to use for strings
		 */
		template<class char_t>
		struct Session
		{
			//! Hostname of the server
			std::basic_string<char_t> host;
			//! User agent string
			std::basic_string<char_t> user_agent;
			//! Content types the client accepts
			std::basic_string<char_t> accept_content_types;
			//! Languages the client accepts
			std::basic_string<char_t> accept_languages;
			//! Character sets the clients accepts
			std::basic_string<char_t> accept_charsets;
			//! Referral URL. Percent symbol escaped bytes are converted to their actual value.
			std::basic_string<char_t> referer;
			//! Content type of data sent from client
			std::basic_string<char_t> content_type;
			//! Query string appended to the URL submitted by the client. Percent symbol escaped bytes are converted to their actual value.
			std::basic_string<char_t> query_string;
			//! Cookie string sent from the client
			std::basic_string<char_t> cookies;
			//! HTTP root directory
			std::basic_string<char_t> root;
			//! Filename of script relative to the HTTP root directory
			std::basic_string<char_t> script_name;
			//! The etag the client assumes this document should have
			int etag;
			//! How many seconds the connection should be kept alive
			int keep_alive;
			//! Length of content to be received from the client (post data)
			unsigned int content_length;
			//! IP address of the server
			Address server_address;
			//! IP address of the client
			Address remote_address;
			//! TCP port used by the server
			uint16_t server_port;
			//! TCP port used by the client
			uint16_t remote_port;
			//! Timestamp the client has for this document
			boost::posix_time::ptime if_modified_since;
			typedef std::map<std::basic_string<char_t>, std::basic_string<char_t> > strmap;
			strmap headers;

			//typedef std::map<std::basic_string<char_t>, std::basic_string<char_t> > Other_data;
			//Other_data other_data;

			typedef std::map<std::basic_string<char_t>, Post<char_t> > Posts;
			//! STL container associating POST objects with their name
			Posts post;
			//! STL container associating GET objects with their name
			strmap get;
			strmap cookie;

			//! Parses Fast_cGI parameter data into the data structure
			/*!
			 * This function will take the body of a Fast_cGI parameter record and parse
			 * the data into the data structure. data should equal the first character of
			 * the records body with size being it's content length.
			 *
			 * @param[in] data Pointer to the first byte of parameter data
			 * @param[in] size Size of data in bytes
			 */
			bool fill(const char* data, size_t size);
			//! Parses raw http post data into the posts object
			/*!
			 * This function will take arbitrarily divided chunks of raw http post
			 * data and parse them into the posts container of Post objects. data should
			 * equal the first bytes of the Fast_cGI IN records body with size being it's
			 * content length.
			 *
			 * @param[in] data Pointer to the first byte of post data
			 * @param[in] size Size of data in bytes
			 */
			void fill_posts(const char* data, size_t size);

			//! Clear the post buffer
			void clear_post_buffer() { post_buffer.reset(); post_buffer_size=0; }

			static void parse_url(const std::basic_string<char_t> &, std::map<std::basic_string<char_t>, std::basic_string<char_t> > &);

		private:
			//! Raw string of characters representing the post boundary
			boost::scoped_array<char> boundary;
			//! Size of boundary
			size_t boundary_size;

			//! Buffer for processing post data
			boost::scoped_array<char> post_buffer;
			//! Size of data in post_buffer
			size_t post_buffer_size;
		};

		//! Convert a char string to a std::wstring
		/*!
		 * @param[in] data First byte in char string
		 * @param[in] size Size in bytes of the string (no null terminator)
		 * @param[out] string Reference to the wstring that should be modified
		 * @return Returns true on success, false on failure
		 */
		bool char_to_string(const char* data, size_t size, std::wstring& string);
		//! Convert a char string to a std::string
		/*!
		 * @param[in] data First byte in char string
		 * @param[in] size Size in bytes of the string (no null terminator)
		 * @param[out] string Reference to the string that should be modified
		 * @return Returns true on success, false on failure
		 */
		inline bool char_to_string(const char* data, size_t size, std::string& string) { string.assign(data, size); return true; }
		//! Convert a char string to an integer
		/*!
		 * This function is very similar to std::atoi() except that it takes start/end values
		 * of a non null terminated char string instead of a null terminated string. The first
		 * character must be either a number or a minus sign (-). As soon as the end is reached
		 * or a non numerical character is reached, the result is tallied and returned.
		 *
		 * @param[in] start Pointer to the first byte in the string
		 * @param[in] end Pointer to the last byte in the string + 1
		 * @return Integer value represented by the string
		 */
		int atoi(const char* start, const char* end);

		//! Finds the value associated with a name in an 'name="value"' string
		/*!
		 * Note that the quotation marks are removed from the value. If no value is found, 
		 * then string is left unchanged.
		 *
		 * @param[in] name Pointer to a null terminated string containing the name
		 * @param[in] start Pointer to the first byte of data to look in
		 * @param[in] end Pointer to the last byte of data to look in + 1
		 * @param[out] string Reference to the string the value should be stored in.
		 */
		template<class char_t>
		bool parse_xml_value(const char* const name, const char* start, const char* end, std::basic_string<char_t>& string);

		//! Convert a string with percent escaped byte values to their actual values
		/*!
		 *	Since converting a percent escaped string to actual values can only make it shorter, 
		 *	it is safe to assume that the return value will always be smaller than size. It is
		 *	thereby a safe move to make the destination block of memory the same size as the source.
		 *
		 * @param[in] source Pointer to the first character in the percent escaped string
		 * @param[in] size Size in bytes of the data pointed to by source (no null termination)
		 * @param[out] destination Pointer to the section of memory to write the converted string to
		 * @return Actual size of the new string
		 */
		template<class char_t>
		int percent_escaped_to_real_bytes(const char_t* source, char_t* dest, size_t size);
	}
}

#endif
