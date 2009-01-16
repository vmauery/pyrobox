//! \file protocol.hpp Defines Fast_cGI protocol
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


#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <map>
#include <string>
#include <endian.h>
#include <stdint.h>

#include <boost/shared_array.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	namespace Protocol
	{
		//! The request ID of a Fast_cGI request
		typedef uint16_t Request_id;
		
		//! A full ID value for a Fast_cGI request
		/*!
		 * Because each Fast_cGI request has a Request_iD and a file descriptor
		 * associated with it, this class defines an ID value that encompasses
		 * both. The file descriptor is stored internally as a 16 bit unsigned
		 * integer in order to keep the data structures size at 32 bits for
		 * optimized indexing.
		 */
		struct Full_id
		{
			//! Construct from a Fast_cGI Request_iD and a file descriptor
			/*!
			 * The constructor builds upon a Request_iD and the file descriptor
			 * it is communicating through.
			 *
			 * @param  [in] fcgi_id_ The Fast_cGI request ID
			 * @param [in] fd_ The file descriptor
			 */
			Full_id(Request_id fcgi_id_, int fd_): fcgi_id(fcgi_id_), fd(fd_) { } 
			Full_id() { }
			//! Fast_cGI Request ID
			Request_id fcgi_id;
			//! Associated File Descriptor
			uint16_t fd;
		};
		
		//! Defines the types of records within the Fast_cGI protocol
		enum Record_type { BEGIN_REQUEST=1, ABORT_REQUEST=2, END_REQUEST=3, PARAMS=4, IN=5, OUT=6, ERR=7, DATA=8, GET_VALUES=9, GET_VALUES_RESULT=10, UNKNOWN_TYPE=11 };
		
		//! Defines text labels for the Record_type values
		extern const char* record_type_labels[];
	}
}
		

#include <exceptions.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! Defines aspects of the Fast_cGI %Protocol
	/*!
	 * The %Protocol namespace defines the data structures and constants
	 * used by the Fast_cGI protocol version 1. All data has been modelled
	 * after the official Fast_cGI protocol specification located at
	 * http://www.fastcgi.com/devkit/doc/fcgi-spec.html
	 */
	namespace Protocol
	{
		//! The version of the Fast_cGI protocol that this adheres to
		const int version=1;

		//! All Fast_cGI records will be a multiple of this many bytes
		const int chunk_size=8;

		//! Defines the possible roles a Fast_cGI application may play
		enum Role { RESPONDER=1, AUTHORIZER=2, FILTER=3 };

		//! Possible statuses a request may declare when complete
		enum Protocol_status { REQUEST_COMPLETE=0, CANT_MPX_CONN=1, OVERLOADED=2, UNKNOWN_ROLE=3 };

		//!Compare between two Full_id variables
		/*!
		 * This comparator casts the structures as 32 bit integers and compares them as such.
		 */
		inline bool operator>(const Full_id& x, const Full_id& y) { return *(uint32_t*)&x.fcgi_id > *(uint32_t*)&y.fcgi_id; }

		//!Compare between two Full_id variables
		/*!
		 * This comparator casts the structures as 32 bit integers and compares them as such.
		 */
		inline bool operator<(const Full_id& x, const Full_id& y) { return *(uint32_t*)&x.fcgi_id < *(uint32_t*)&y.fcgi_id; }

		//!Compare between two Full_id variables
		/*!
		 * This comparator casts the structures as 32 bit integers and compares them as such.
		 */
		inline bool operator==(const Full_id& x, const Full_id& y) { return *(uint32_t*)&x.fcgi_id == *(uint32_t*)&y.fcgi_id; }

		//!Read in a big endian value
		/*!
		 * This function will read in a big endian value of type T. Should the
		 * system compiled for use big endian first, it simple returns the value passed
		 * to it. Should the system compiled for use little endian first, it will return
		 * a value with the endianess reversed.
		 * @param value Value with big endian first
		 * @return Value conforming to the endianess of the system
		 */
#if __BYTE_ORDER == __LITTLE_ENDIAN
		template<class T> T read_big_endian(T value)
		{
			T result;
			char* p_value=(char*)&value-1;
			char* p_value_end=p_value+sizeof(T);
			char* p_result=(char*)&result+sizeof(T);
			while(p_value!=p_value_end)
				*--p_result=*++p_value;
			return result;
		}
#elif __BYTE_ORDER == __BIG_ENDIAN
		template<class T> T read_big_endian(T value)
		{
			return value;
		}
#endif

		//!Data structure used as the header for Fast_cGI records
		/*!
		 * This structure defines the header used in Fast_cGI records. It can be casted 
		 * to and from raw 8 byte blocks of data and transmitted/received as is. The
		 * endianess and order of data is kept correct through the accessor member functions.
		 */
		class Header
		{
		public:
			//!Set the version field of the record header
			/*!
			 * @param[in] version_ Fast_cGI protocol version number
			 */
			void set_version(uint8_t version_) { version=version_; }

			//!Get the version field of the record header
			/*!
			 * @return version Fast_cGI protocol version number
			 */
			int get_version() const { return version; }

			//!Set the record type in the header
			/*!
			 * @param[in] type_ Record type
			 */
			void set_type(Record_type type_) { type=static_cast<uint8_t>(type_); }

			//!Get the record type in the header
			/*!
			 * @return Record type
			 */
			Record_type get_type() const { return static_cast<Record_type>(type); }

			//!Set the request ID field in the record header
			/*!
			 * @param[in] request_id_ The records request ID
			 */
			void set_request_id(Request_id request_id_) { *(uint16_t*)&request_id_b1=read_big_endian(request_id_); }

			//!Get the request ID field in the record header
			/*!
			 * @return The records request ID
			 */
			Request_id get_request_id() const { return read_big_endian(*(uint16_t*)&request_id_b1); }

			//!Set the content length field in the record header
			/*!
			 * @param[in] content_length_ The records content length
			 */
			void set_content_length(uint16_t content_length_) { *(uint16_t*)&content_length_b1=read_big_endian(content_length_); }

			//!Get the content length field in the record header
			/*!
			 * @return The records content length
			 */
			int get_content_length() const { return read_big_endian(*(uint16_t*)&content_length_b1); }

			//!Set the padding length field in the record header
			/*!
			 * @param[in] padding_length_ The records padding length
			 */
			void set_padding_length(uint8_t padding_length_) { padding_length=padding_length_; }

			//!Get the padding length field in the record header
			/*!
			 * @return The records padding length
			 */
			int get_padding_length() const { return padding_length; }
		private:
			//! Fast_cGI version number
			uint8_t version;
			//! Record type
			uint8_t type;
			//! Request ID most significant byte
			uint8_t request_id_b1;
			//! Request ID least significant byte
			uint8_t request_id_b0;
			//! Content length most significant byte
			uint8_t content_length_b1;
			//! Content length least significant byte
			uint8_t content_length_b0;
			//! Length of record padding
			uint8_t padding_length;
			//! Reseved for future use and header padding
			uint8_t reserved;
		};

		
		//!Data structure used as the body for Fast_cGI records with a Record_type of BEGIN_REQUEST
		/*!
		 * This structure defines the body used in Fast_cGI BEGIN_REQUEST records. It can be casted 
		 * from raw 8 byte blocks of data and received as is. A BEGIN_REQUEST record is received
		 * when the other side wished to make a new request.
		 */
		class Begin_request
		{
		public:
			//!Get the role field from the record body
			/*!
			 * @return The expected Role that the request will play
			 */
			Role get_role() const { return static_cast<Role>(read_big_endian(*(uint16_t*)&role_b1)); }

			//!Get keep alive value from the record body
			/*!
			 * If this value is false, the socket should be closed on our side when the request is complete.
			 * If true, the other side will close the socket when done and potentially reuse the socket and
			 * multiplex other requests on it.
			 *
			 * @return Boolean value as to whether or not the connection is kept alive
			 */
			bool get_keep_conn() const { return flags & keep_conn_bit; }
		private:
			//! Flag bit representing the keep alive value
			static const int keep_conn_bit = 1;

			//! Role value most significant byte
			uint8_t role_b1;
			//! Role value least significant byte
			uint8_t role_b0;
			//! Flag value
			uint8_t flags;
			//! Reseved for future use and body padding
			uint8_t reserved[5];
		};

		//!Data structure used as the body for Fast_cGI records with a Record_type of UNKNOWN_TYPE
		/*!
		 * This structure defines the body used in Fast_cGI UNKNOWN_TYPE records. It can be casted 
		 * to raw 8 byte blocks of data and transmitted as is. An UNKNOWN_TYPE record is sent as
		 * a reply to record types that are not recognized.
		 */
		class Unknown_type
		{
		public:
			//!Set the record type that is unknown
			/*!
			 * @param[in] type_ The unknown record type
			 */
			void set_type(Record_type type_) { type=static_cast<uint8_t>(type_); }
		private:
			//! Unknown record type
			uint8_t type;
			//! Reseved for future use and body padding
			uint8_t reserved[7];
		};

		//!Data structure used as the body for Fast_cGI records with a Record_type of END_REQUEST
		/*!
		 * This structure defines the body used in Fast_cGI END_REQUEST records. It can be casted 
		 * to raw 8 byte blocks of data and transmitted as is. An END_REQUEST record is sent when
		 * this side wishes to terminate a request. This can be simply because it is complete or
		 * because of a problem.
		 */
		class End_request
		{
		public:
			//!Set the requests return value
			/*!
			 * This is an integer value representing what would otherwise be the return value in a
			 * normal CGI application.
			 *
			 * @param[in] status The return value
			 */
			void set_app_status(int status) { *(int*)&app_status_b3=read_big_endian(status); }

			//!Set the reason for termination
			/*!
			 * This value is one of Protocol_status and represents the reason for termination.
			 *
			 * @param[in] status The requests status
			 */
			void set_protocol_status(Protocol_status status) { protocol_status=static_cast<uint8_t>(status); }
		private:
			//! Return value most significant byte
			uint8_t app_status_b3;
			//! Return value second most significant byte
			uint8_t app_status_b2;
			//! Return value third most significant byte
			uint8_t app_status_b1;
			//! Return value least significant byte
			uint8_t app_status_b0;
			//! Requests Status
			uint8_t protocol_status;
			//! Reseved for future use and body padding
			uint8_t reserved[3];
		};

		//!Process the body of a Fast_cGI parameter record
		/*!
		 * Takes the body of a Fast_cGI record of type PARAMETER and parses  it. You end
		 * up with a pointer/size for both the name and value of the parameter.
		 *
		 * @param[in] data Pointer to the record body
		 * @param[out] name Reference to a pointer that will be pointed to the first byte of the parameter name
		 * @param[out] name_size Reference to a value to will be given the size in bytes of the parameter name
		 * @param[out] value Reference to a pointer that will be pointed to the first byte of the parameter value
		 * @param[out] value_size Reference to a value to will be given the size in bytes of the parameter value
		 */
		bool process_param_header(const char* data, size_t data_size, const char*& name, size_t& name_size, const char*& value, size_t& value_size);

		
		//!Used for the reply of Fast_cGI management records of type GET_VALUES
		/*!
		 * This class template is an efficient tool for replying to GET_VALUES management
		 * records. The structure represents a complete record (body+header) of a name-value pair to be
		 * sent as a reply to a management value query. The templating allows the structure
		 * to be exactly the size that is needed so it can be casted to raw data and transmitted
		 * as is. Note that the name and value lengths are left as single bytes so they are limited
		 * in range from 0-127.
		 *
		 * @tparam NAMELENGTH Length of name in bytes (0-127). Null terminator not included.
		 * @tparam VALUELENGTH Length of value in bytes (0-127). Null terminator not included.
		 * @tparam PADDINGLENGTH Length of padding at the end of the record. This is needed to keep
		 * the record size a multiple of chunk_size.
		 */
		template<int NAMELENGTH, int VALUELENGTH, int PADDINGLENGTH>
		struct Management_reply
		{
		private:
			//! Management records header
			Header header;
			//! Length in bytes of name
			uint8_t name_length;
			//! Length in bytes of value
			uint8_t value_length;
			//! Name data
			uint8_t name[NAMELENGTH];
			//! Value data
			uint8_t value[VALUELENGTH];
			//! Padding data
			uint8_t padding[PADDINGLENGTH];
		public:
			//! Construct the record based on the name data and value data
			/*!
			 * A full record is constructed from the name-value data. After
			 * construction the structure can be casted to raw data and transmitted
			 * as is. The size of the data arrays pointed to by name_ and value_ are
			 * assumed to correspond with the NAMELENGTH and PADDINGLENGTH template
			 * parameters passed to the class.
			 *
			 * @param[in] name_ Pointer to name data
			 * @param[in] value_ Pointer to value data
			 */
			Management_reply(const char* name_, const char* value_): name_length(NAMELENGTH), value_length(VALUELENGTH)
			{
				for(int i=0; i<NAMELENGTH; i++) name[i]=*(name_+i);
				for(int i=0; i<VALUELENGTH; i++) value[i]=*(value_+i);
				header.set_version(version);
				header.set_type(GET_VALUES_RESULT);
				header.set_request_id(0);
				header.set_content_length(NAMELENGTH+VALUELENGTH);
				header.set_padding_length(PADDINGLENGTH);
			}
		};

		//! Reply record that will be sent when asked the maximum allowed file descriptors open at a time
		extern Management_reply<14, 2, 8> max_conns_reply;
		//! Reply record that will be sent when asked the maximum allowed requests at a time
		extern Management_reply<13, 2, 1> max_reqs_reply;
		//! Reply record that will be sent when asked if requests can be multiplexed over a single connections
		extern Management_reply<15, 1, 8> mpxs_conns_reply;
	}

	//! Data structure used to pass messages within the fastcgi++ task management system
	/*!
	 * This data structure is crucial to all operation in the Fast_cGI library as all
	 * data passed to requests must be encapsulated in this data structure. A type value
	 * of 0 means that the message is a Fast_cGI record and will be processed at a low
	 * level by the library. Any other type value and the message will be passed up to
	 * the user to be processed. The data may contain any data that can be casted to/from
	 * a raw character array. The size obviously represents the exact size of the data
	 * section.
	 */
	struct Message
	{
		//! Type of message. A 0 means Fast_cGI record. Anything else is open.
		int type;
		//! Size of the data section.
		size_t size;
		//! Pointer to the raw data being passed along with the message.
		boost::shared_array<char> data;
	};
}

#endif
