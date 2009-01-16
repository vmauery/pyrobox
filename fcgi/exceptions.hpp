//! \file exceptions.hpp Defines fastcgi++ exceptions
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


#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>
#include <string>

#include <protocol.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! Namespace that defines fastcgi++ related exceptions
	namespace Exceptions
	{
		//! General fastcgi++ exception
		class Exception: public std::exception
		{
		public:
			virtual const char* what() const throw() =0;
			~Exception() throw() {}
		};
		
		//! General fastcgi++ request exception
		class Request: public Exception
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] id_ ID value for the request that generated the exception
			 */
			Request(Protocol::Full_id id_): id(id_) { }
			~Request() throw() {}
			
			virtual const char* what() const throw() =0;
			Protocol::Full_id get_id() const throw() { return id; }
		protected:
			//! ID value for the request that generated the exception
			Protocol::Full_id id;
		};
		
		//! %Exception for parameter decoding errors
		class Param: public Request
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] id_ ID value for the request that generated the exception
			 */
			Param(Protocol::Full_id id_);
			~Param() throw() {}
			virtual const char* what() const throw() { return msg.c_str(); }
		private:
			//! Error message associated with the exception
			std::string msg;
		};
		
		//! %Exception for output stream processing
		class Stream: public Request
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] id_ ID value for the request that generated the exception
			 */
			Stream(Protocol::Full_id id_);
			~Stream() throw() {}
			virtual const char* what() const throw() { return msg.c_str(); }
		private:
			//! Error message associated with the exception
			std::string msg;
		};
		
		//! %Exception for reception of records out of order
		class Record_out_of_order: public Request
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] id_ ID value for the request that generated the exception
			 * @param[in] expected_record_ Type of record that was expected
			 * @param[in] recieved_record_ Type of record that was recieved
			 */
			Record_out_of_order(Protocol::Full_id id_, Protocol::Record_type expected_record_, Protocol::Record_type recieved_record_);
			~Record_out_of_order() throw() {}
			virtual const char* what() const throw() { return msg.c_str(); }
			Protocol::Record_type get_expected_record() const throw() { return expected_record; }
			Protocol::Record_type get_recieved_record() const throw() { return recieved_record; }
		private:
			//! Type of record that was expected
			Protocol::Record_type expected_record;
			//! Type of record that was recieved
			Protocol::Record_type recieved_record;
			//! Error message associated with the exception
			std::string msg;
		};
		
		//! General exception for socket related errors
		class Socket: public Exception
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] fd_ File descriptor of socket
			 * @param[in] erno_ Associated errno
			 */
			Socket(int fd_, int erno_): fd(fd_), erno(erno_) { }
			~Socket() throw() {}
			virtual const char* what() const throw() =0;
			int get_fd() const throw() { return fd; }
			int get_errno() const throw() { return erno; }
		protected:
			//! File descriptor of socket
			int fd;
			//! Associated errno
			int erno;
		};
		
		//! %Exception for write errors to sockets
		class Socket_write: public Socket
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] fd_ File descriptor of socket
			 * @param[in] erno_ Associated errno
			 */
			Socket_write(int fd_, int erno_);
			~Socket_write() throw() {}
			virtual const char* what() const throw() { return msg.c_str(); }
		private:
			//! Error message associated with the exception
			std::string msg;
		};
		
		//! %Exception for read errors to sockets
		class Socket_read: public Socket
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] fd_ File descriptor of socket
			 * @param[in] erno_ Associated errno
			 */
			Socket_read(int fd_, int erno_);
			~Socket_read() throw() {}
			virtual const char* what() const throw() { return msg.c_str(); }
		private:
			//! Error message associated with the exception
			std::string msg;
		};
		
		//! %Exception for poll() errors
		class Poll: public Exception
		{
		public:
			//! Sole Constructor
			/*!
			 * @param[in] erno_ Associated errno
			 */
			Poll(int erno_);
			~Poll() throw() {}
			virtual const char* what() const throw() { return msg.c_str(); }
			int get_erno() const throw() { return erno; }
		private:
			//! Associated errno
			int erno;
			//! Error message associated with the exception
			std::string msg;
		};
	}
}

#endif
