//! \file manager.hpp Defines the Fastcgipp::Manager class
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


#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <map>
#include <string>
#include <queue>
#include <algorithm>
#include <cstring>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <signal.h>

#include <exceptions.hpp>
#include <protocol.hpp>
#include <transceiver.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! General task and protocol management class
	/*!
	 * Handles all task and protocol management, creation/destruction
	 * of requests and passing of messages to requests. The template argument
	 * should be a class type derived from the Request class with at least the
	 * response() function defined. To operate this class all that needs to be
	 * done is creating an object and calling handler() on it.
	 *
	 * @tparam T Class that will handle individual requests. Should be derived from
	 * the Request class.
	 */
	template<typename T>
	class Manager
	{
	public:
		//! Construct from a file descriptor
		/*!
		 * The only piece of data required to construct a %Manager object is a
		 * file descriptor to listen on for incoming connections. By default
		 * mod_fastcgi sets up file descriptor 0 to do this so it is the value
		 * passed by default to the constructor. The only time it would be another
		 * value is if an external Fast_cGI server was defined.
		 *
		 * @param [in] fd File descriptor to listen on.
		 */
		Manager(int fd=0): transceiver(fd, boost::bind(&Manager::push, boost::ref(*this), _1, _2)), asleep(false), terminate_bool(false), stop_bool(false) { setup_signals(); instance=this; }

		~Manager() { instance=0; }

		//! General handling function to be called after construction
		/*!
		 * This function will loop continuously manager tasks and Fast_cGI
		 * requests until either the stop() function is called (obviously from another
		 * thread) or the appropriate signals are caught.
		 *
		 * @sa setup_signals()
		 */
		void handler();

		//! Passes messages to requests
		/*!
		 * Whenever a message needs to be passed to a request, it must be done through
		 * this function. %Requests are associated with their Protocol::Full_id value so
		 * that and the message itself is all that is needed. Calling this function from
		 * another thread is safe. Although this function can be called from outside
		 * the fastcgi++ library, the Request class contains a callback function based
		 * on this that is more usable. An id with a Protocol::Request_id of 0 means the message
		 * is destined for the %Manager itself. Should a message by passed with an id that doesn't
		 * exist, it will be discarded.
		 *
		 * @param[in] id The id of the request the message should go to
		 * @param[in] message The message itself
		 *
		 * @sa Request::callback
		 */
		void push(Protocol::Full_id id, Message message);

		//! Halter for the handler() function
		/*!
		 * This function is intended to be called from a thread separate from the handler()
		 * in order to halt it. It should also be called by a signal handler in the case of
		 * of a SIGTERM. Once %handler() has been halted it may be re-called to pick up
		 * exactly where it left off without any data loss.
		 *
		 * @sa setup_signals()
		 * @sa signal_handler()
		 */
		void stop();

		
		//! Configure the handlers for POSIX signals
		/*!
		 * By calling this function appropriate handlers will be set up for SIGPIPE, SIGUSR1 and
		 * SIGTERM. It is called by default upon construction of a Manager object. Should
		 * the user want to override these handlers, it should be done post-construction.
		 *
		 * @sa signal_handler()
		 */
		void setup_signals();
	private:
		//! Handles low level communication with the other side
		Transceiver transceiver;

		//! Queue type for pending tasks
		/*!
		 * This is merely a derivation of a std::queue<Protocol::Full_id> and a
		 */
		typedef std::queue<Protocol::Full_id> Tasks;
		//! Queue for pending tasks
		/*!
		 * This contains a queue of Protocol::Full_id that need their handlers called.
		 */
		Tasks tasks;

		//! Associative container type for active requests
		/*!
		 * This is merely a derivation of a std::map<Protocol::Full_id, boost::shared_ptr<T> > and a
		 */
		typedef std::map<Protocol::Full_id, boost::shared_ptr<T> > Requests;
		//! Associative container type for active requests
		/*!
		 * This container associated the Protocol::Full_id of each active request with a pointer
		 * to the actual Request object.
		 */
		Requests requests;

		//! A queue of messages for the manager itself
		std::queue<Message> messages;

		//! Handles management messages
		/*!
		 * This function is called by handler() in the case that a management message is recieved.
		 * Although the request id of a management record is always 0, the Protocol::Full_id associated
		 * with the message is passed to this function to keep track of it's associated
		 * file descriptor.
		 *
		 * @param[in] id Full_id associated with the messsage.
		 */
		inline void local_handler(Protocol::Full_id id);

		//! Indicated whether or not the manager is currently in sleep mode
		bool asleep;

		//! Boolean value indicating that handler() should halt
		/*!
		 * @sa stop()
		 */
		bool stop_bool;
		//! Boolean value indication that handler() should terminate
		/*!
		 * @sa terminate()
		 */
		bool terminate_bool;

		//! General function to handler POSIX signals
		static void signal_handler(int signum);
		//! Pointer to the %Manager object
		static Manager<T>* instance;
		//! Terminator for the handler() function
		/*!
		 * This function is intended to be called from  a signal handler in the case of
		 * of a SIGUSR1. It is similar to stop() except that handler() will wait until
		 * all requests are complete before halting.
		 *
		 * @sa setup_signals()
		 * @sa signal_handler()
		 */
		inline void terminate();
	};
}

template<class T>
Fastcgipp::Manager<T>* Fastcgipp::Manager<T>::instance=0;

template<class T>
void Fastcgipp::Manager<T>::terminate()
{
	terminate_bool=true;
	transceiver.terminate();
}

template<class T>
void Fastcgipp::Manager<T>::stop()
{
	stop_bool=true;
	transceiver.wake();
	transceiver.stop();
}

template<class T>
void Fastcgipp::Manager<T>::signal_handler(int signum)
{
	switch (signum) {
		case SIGTERM:
		case SIGPIPE:
		case SIGUSR1:
			/*
			if (instance)
				instance->stop();
			break;
			*/
		case SIGABRT:
		case SIGINT:
		case SIGQUIT:
			if (instance)
				instance->terminate();
			break;
	}
}

// XXX register callbacks for other things to be notified on shutdown

template<class T>
void Fastcgipp::Manager<T>::setup_signals()
{
	struct sigaction sig_action;
	sig_action.sa_handler=Fastcgipp::Manager<T>::signal_handler;

	sigaction(SIGPIPE, &sig_action, NULL);
	sigaction(SIGUSR1, &sig_action, NULL);
	sigaction(SIGTERM, &sig_action, NULL);
	sigaction(SIGINT, &sig_action, NULL);
	sigaction(SIGQUIT, &sig_action, NULL);
	sigaction(SIGHUP, &sig_action, NULL);
	sigaction(SIGABRT, &sig_action, NULL);
}

template<class T>
void Fastcgipp::Manager<T>::push(Protocol::Full_id id, Message message)
{
	using namespace std;
	using namespace Protocol;
	using namespace boost;

	if(id.fcgi_id)
	{
		typename Requests::iterator it(requests.find(id));
		if(it!=requests.end())
		{
			it->second->messages.push(message);
			tasks.push(id);
		}
		else if(!message.type)
		{
			Header& header=*(Header*)message.data.get();
			if(header.get_type()==BEGIN_REQUEST)
			{
				Begin_request& body=*(Begin_request*)(message.data.get()+sizeof(Header));
				boost::shared_ptr<T>& request = requests[id];
				request.reset(new T);
				request->set(id, transceiver, body.get_role(), !body.get_keep_conn(), boost::bind(&Manager::push, boost::ref(*this), id, _1));
			}
			else
				return;
		}
	}
	else
	{
		messages.push(message);
		tasks.push(id);
	}

	if(asleep)
		transceiver.wake();
}

template<class T>
void Fastcgipp::Manager<T>::handler()
{
	using namespace std;
	using namespace boost;

	while (1) {
		if(stop_bool) {
			stop_bool=false;
			return;
		}

		bool sleep=transceiver.handler();
		if(terminate_bool) {
			if(requests.empty() && sleep) {
				terminate_bool=false;
				return;
			}
		}

		if(tasks.empty()) {
			asleep=true;
			if (sleep) {
				transceiver.sleep();
			}
			asleep=false;
			continue;
		}

		Protocol::Full_id id=tasks.front();
		tasks.pop();

		if(id.fcgi_id==0) {
			local_handler(id);
		} else {
			typename map<Protocol::Full_id, boost::shared_ptr<T> >::iterator it(requests.find(id));
			if(it!=requests.end() && it->second->handler()) {
				requests.erase(it);
			}
		}
	}
}

template<class T>
void Fastcgipp::Manager<T>::local_handler(Protocol::Full_id id)
{
	using namespace std;
	using namespace Protocol;
	Message message(messages.front());
	messages.pop();
	
	if(!message.type)
	{
		const Header& header=*(Header*)message.data.get(); 
		switch(header.get_type())
		{
			case GET_VALUES:
			{
				size_t name_size;
				size_t value_size;
				const char* name;
				const char* value;
				process_param_header(message.data.get()+sizeof(Header), header.get_content_length(), name, name_size, value, value_size);
				if(name_size==14 && !memcmp(name, "FCGI_MAX_CONNS", 14))
				{
					Block buffer(transceiver.request_write(sizeof(max_conns_reply)));
					memcpy(buffer.data, (const char*)&max_conns_reply, sizeof(max_conns_reply));
					transceiver.secure_write(sizeof(max_conns_reply), id, false);
				}
				else if(name_size==13 && !memcmp(name, "FCGI_MAX_REQS", 13))
				{
					Block buffer(transceiver.request_write(sizeof(max_reqs_reply)));
					memcpy(buffer.data, (const char*)&max_reqs_reply, sizeof(max_reqs_reply));
					transceiver.secure_write(sizeof(max_reqs_reply), id, false);
				}
				else if(name_size==15 && !memcmp(name, "FCGI_MPXS_CONNS", 15))
				{
					Block buffer(transceiver.request_write(sizeof(mpxs_conns_reply)));
					memcpy(buffer.data, (const char*)&mpxs_conns_reply, sizeof(mpxs_conns_reply));
					transceiver.secure_write(sizeof(mpxs_conns_reply), id, false);
				}

				break;
			}

			default:
			{
				Block buffer(transceiver.request_write(sizeof(Header)+sizeof(Unknown_type)));

				Header& send_header=*(Header*)buffer.data;
				send_header.set_version(version);
				send_header.set_type(UNKNOWN_TYPE);
				send_header.set_request_id(0);
				send_header.set_content_length(sizeof(Unknown_type));
				send_header.set_padding_length(0);

				Unknown_type& send_body=*(Unknown_type*)(buffer.data+sizeof(Header));
				send_body.set_type(header.get_type());

				transceiver.secure_write(sizeof(Header)+sizeof(Unknown_type), id, false);

				break;
			}
		}
	}
}

#endif
