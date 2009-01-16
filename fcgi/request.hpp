//! \file request.hpp Defines the Fastcgipp::Request class
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


#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <queue>
#include <map>
#include <string>
#include <locale>

#include <boost/shared_array.hpp>
#include <boost/function.hpp>

#include <protocol.hpp>
#include <exceptions.hpp>
#include <transceiver.hpp>
#include <fcgistream.hpp>
#include <http.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! %Request handling class
	/*!
	 * Derivations of this class will handle requests. This
	 * includes building the session data, processing post/get data,
	 * fetching data (files, database), and producing a response.
	 * Once all client data is organized, response() will be called.
	 * At minimum, derivations of this class must define response().
	 *
	 * If you want to use UTF-8 encoding pass wchar_t as the template
	 * argument, use setloc() to setup a UTF-8 locale and use wide 
	 * character unicode internally for everything. If you want to use
	 * a 8bit character set encoding pass char as the template argument and
	 * setloc() a locale with the corresponding character set.
	 *
	 * @tparam char_t Character type for internal processing (wchar_t or char)
	 */
	template<class char_t> class Request
	{
	public:
		//! Initializes what it can. set() must be called by Manager before the data is usable.
		Request(): state(Protocol::PARAMS)  { setloc(std::locale::classic()); out.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit); session.clear_post_buffer(); }

	protected:
		//! Structure containing all HTTP session data
		Http::Session<char_t> session;

		// To dump data into the stream without it being code converted and bypassing the stream buffer call Fcgistream::dump(char* data, size_t size)
		// or Fcgistream::dump(std::basic_istream<char>& stream)
		
		//! Standard output stream to the client
		/*!
		 * To dump data directly through the stream without it being code converted and bypassing the stream buffer call Fcgistream::dump()
		 */
		Fcgistream<char_t, std::char_traits<char_t> > out;

		//! Output stream to the HTTP server error log
		/*!
		 * To dump data directly through the stream without it being code converted and bypassing the stream buffer call Fcgistream::dump()
		 */
		Fcgistream<char_t, std::char_traits<char_t> > err;

		//! Response generator
		/*!
		 * This function is called by handler() once all request data has been received from the other side or if a
		 * Message not of a Fast_cGI type has been passed to it. The function shall return true if it has completed
		 * the response and false if it has not (waiting for a callback message to be sent).
		 *
		 * @return Boolean value indication completion (true means complete)
		 * @sa callback
		 */
		virtual bool response() =0;

		//! Generate a data input response
		/*!
		 * This function exists should the library user wish to do something like generate a partial response based on
		 * bytes received from the client. The function is called by handler() every time a Fast_cGI IN record is received.
		 * The function has no access to the data, but knows exactly how much was received based on the value that was passed.
		 * Note this value represents the amount of data received in the individual record, not the total amount received in
		 * the session. If the library user wishes to have such a value they would have to keep a tally of all size values
		 * passed.
		 *
		 * @param[in] bytes_received Amount of bytes received in this Fast_cGI record
		 */
		virtual void in_handler(int bytes_received) { };

		//! The locale associated with the request. Should be set with setloc(), not directly.
		std::locale loc;

		//! The message associated with the current handler() call.
		/*!
		 * This is only of use to the library user when a non Fast_cGI (type=0) Message is passed
		 * by using the requests callback.
		 *
		 * @sa callback
		 */
		Message message;

		//! Set the requests locale
		/*!
		 * This function both sets loc to the locale passed to it and imbues the locale into the
		 * out and err stream. The user should always call this function as opposed to setting the
		 * locales directly is this functions insures the utf8 code conversion is functioning properly.
		 *
		 * @param[in] loc_ New locale
		 * @sa loc
		 * @sa out
		 */
		void setloc(std::locale loc_);

		//! Callback function for dealings outside the fastcgi++ library
		/*!
		 * The purpose of the callback object is to provide a thread safe mechanism for functions and
		 * classes outside the fastcgi++ library to talk to the requests. Should the library
		 * wish to have another thread process or fetch some data, that thread can call this
		 * function when it is finished. It is equivalent to this:
		 *
		 * void callback(Message msg);
		 *
		 *	The sole parameter is a Message that contains both a type value for processing by response()
		 *	and the raw castable data.
		 */
		boost::function<void(Message)> callback;
	private:
		//! Queue type for pending messages
		/*!
		 * This is merely a derivation of a std::queue<Message> and a
		 */
		typedef std::queue<Message> Messages;
		//! A queue of messages to be handler by the request
		Messages messages;

		//! Request Handler
		/*!
		 * This function is called by Manager::handler() to handle messages destined for the request.
		 * It deals with Fast_cGI messages (type=0) while passing all other messages off to response().
		 *
		 * @return Boolean value indicating completion (true means complete)
		 * @sa callback
		 */
		bool handler();
		template <typename T> friend class Manager;
		//! Pointer to the transceiver object that will send data to the other side
		Transceiver* transceiver;
		//! The role that the other side expects this request to play
		Protocol::Role role;
		//! The complete ID (request id & file descriptor) associated with the request
		Protocol::Full_id id;
		//! Boolean value indicating whether or not the file descriptor should be closed upon completion.
		bool kill_con;
		//! What the request is current doing
		Protocol::Record_type state;
		//! Generates an END_REQUEST Fast_cGI record
		void complete();
		//! Set's up the request with the data it needs.
		/*!
		 * This function is an "after-the-fact" constructor that build vital initial data for the request.
		 *
		 * @param[in] id_ Complete ID of the request
		 * @param[in] transceiver_ Transceiver object the request will use
		 * @param[in] role_ The role that the other side expects this request to play
		 * @param[in] kill_con_ Boolean value indicating whether or not the file descriptor should be closed upon completion
		 * @param[in] callback_ Callback function capable of passing messages to the request
		 */
		void set(Protocol::Full_id id_, Transceiver& transceiver_, Protocol::Role role_, bool kill_con_, boost::function<void(Message)> callback_)
		{
			kill_con=kill_con_;
			id=id_;
			transceiver=&transceiver_;
			role=role_;
			callback=callback_;

			err.set(id_, transceiver_, Protocol::ERR);
			out.set(id_, transceiver_, Protocol::OUT);
		}
	};
}

#endif
