//! \file request.cpp Defines member functions for Fastcgipp::Fcgistream and Fastcgipp::Request
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


#include <request.hpp>
#include <http.hpp>
#include "utf8_codecvt.hpp"
#include <util.hpp>

namespace Fastcgipp
{
	template<class char_t> inline std::locale make_locale(std::locale& loc)
	{
		return loc;
	}
	
	template<> std::locale inline make_locale<wchar_t>(std::locale& loc)
	{
		return std::locale(loc, new utf8Code_cvt::utf8_codecvt_facet);
	}
}

template int Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::empty_buffer();
template int Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::empty_buffer();
template <class char_t, class traits>
int Fastcgipp::Fcgistream<char_t, traits>::Fcgibuf::empty_buffer()
{
	using namespace std;
	using namespace Protocol;
	char_type const* p_stream_pos=this->pbase();
	while(1)
	{{
		size_t count=this->pptr()-p_stream_pos;
		size_t wanted_size=count*sizeof(char_type)+dump_size;
		if(!wanted_size)
			break;

		int remainder=wanted_size%chunk_size;
		wanted_size+=sizeof(Header)+(remainder?(chunk_size-remainder):remainder);
		if(wanted_size>numeric_limits<uint16_t>::max()) wanted_size=numeric_limits<uint16_t>::max();
		Block data_block(transceiver->request_write(wanted_size));
		data_block.size=(data_block.size/chunk_size)*chunk_size;

		mbstate_t cs = mbstate_t();
		char* to_next=data_block.data+sizeof(Header);

		locale loc=this->getloc();
		if(count)
		{
			if(sizeof(char_type)!=sizeof(char))
			{
				if(use_facet<codecvt<char_type, char, mbstate_t> >(loc).out(cs, p_stream_pos, this->pptr(), p_stream_pos, to_next, data_block.data+data_block.size, to_next)==codecvt_base::error)
				{
					pbump(-(this->pptr()-this->pbase()));
					dump_size=0;
					dump_ptr=0;
					throw Exceptions::Stream(id);
				}
			}
			else
			{
				size_t cnt=min(data_block.size-sizeof(Header), count);
				memcpy(data_block.data+sizeof(Header), p_stream_pos, cnt);
				p_stream_pos+=cnt;
				to_next+=cnt;
			}
		}

		size_t dumped_size=min(dump_size, static_cast<size_t>(data_block.data+data_block.size-to_next));
		memcpy(to_next, dump_ptr, dumped_size);
		dump_ptr+=dumped_size;
		dump_size-=dumped_size;
		uint16_t content_length=to_next-data_block.data+dumped_size-sizeof(Header);
		uint8_t content_remainder=content_length%chunk_size;
		
		Header& header=*(Header*)data_block.data;
		header.set_version(version);
		header.set_type(type);
		header.set_request_id(id.fcgi_id);
		header.set_content_length(content_length);
		header.set_padding_length(content_remainder?(chunk_size-content_remainder):content_remainder);

		transceiver->secure_write(sizeof(Header)+content_length+header.get_padding_length(), id, false);	
	}}
	pbump(-(this->pptr()-this->pbase()));
	return 0;
}

template std::streamsize Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::xsputn(const char_type *s, std::streamsize n);
template std::streamsize Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::xsputn(const char_type *s, std::streamsize n);
template <class char_t, class traits>
std::streamsize Fastcgipp::Fcgistream<char_t, traits>::Fcgibuf::xsputn(const char_type *s, std::streamsize n)
{
	std::streamsize remainder=n;
	while(remainder)
	{
		std::streamsize actual=std::min(remainder, this->epptr()-this->pptr());
		std::memcpy(this->pptr(), s, actual*sizeof(char_type));
		this->pbump(actual);
		remainder-=actual;
		if(remainder)
		{
			s+=actual;
			empty_buffer();
		}
	}

	return n;
}

template Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::int_type Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::overflow(Fastcgipp::Fcgistream<char, std::char_traits<char> >::Fcgibuf::int_type c = traits_type::eof());
template Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::int_type Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::overflow(Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::Fcgibuf::int_type c = traits_type::eof());
template <class char_t, class traits>
typename Fastcgipp::Fcgistream<char_t, traits>::Fcgibuf::int_type Fastcgipp::Fcgistream<char_t, traits>::Fcgibuf::overflow(Fastcgipp::Fcgistream<char_t, traits>::Fcgibuf::int_type c)
{
	if(empty_buffer() < 0)
		return traits_type::eof();
	if(!traits_type::eq_int_type(c, traits_type::eof()))
		return sputc(c);
	else
		return traits_type::not_eof(c);
}

template void Fastcgipp::Request<char>::complete();
template void Fastcgipp::Request<wchar_t>::complete();
template<class char_t> void Fastcgipp::Request<char_t>::complete()
{
	using namespace Protocol;
	out.flush();
	err.flush();

	Block buffer(transceiver->request_write(sizeof(Header)+sizeof(End_request)));

	Header& header=*(Header*)buffer.data;
	header.set_version(version);
	header.set_type(END_REQUEST);
	header.set_request_id(id.fcgi_id);
	header.set_content_length(sizeof(End_request));
	header.set_padding_length(0);
	
	End_request& body=*(End_request*)(buffer.data+sizeof(Header));
	body.set_app_status(0);
	body.set_protocol_status(REQUEST_COMPLETE);

	transceiver->secure_write(sizeof(Header)+sizeof(End_request), id, kill_con);
}

template void Fastcgipp::Fcgistream<char, std::char_traits<char> >::dump(std::basic_istream<char>& stream);
template void Fastcgipp::Fcgistream<wchar_t, std::char_traits<wchar_t> >::dump(std::basic_istream<char>& stream);
template<class char_t, class traits > void Fastcgipp::Fcgistream<char_t, traits>::dump(std::basic_istream<char>& stream)
{
	const size_t buffer_size=32768;
	char buffer[buffer_size];

	while(stream.good())
	{
		stream.read(buffer, buffer_size);
		dump(buffer, stream.gcount());
	}
}

template bool Fastcgipp::Request<char>::handler();
template bool Fastcgipp::Request<wchar_t>::handler();
template<class char_t> bool Fastcgipp::Request<char_t>::handler()
{
	using namespace Protocol;
	using namespace std;

	try
	{
		if(role!=RESPONDER)
		{
			Block buffer(transceiver->request_write(sizeof(Header)+sizeof(End_request)));
			
			Header& header=*(Header*)buffer.data;
			header.set_version(version);
			header.set_type(END_REQUEST);
			header.set_request_id(id.fcgi_id);
			header.set_content_length(sizeof(End_request));
			header.set_padding_length(0);
			
			End_request& body=*(End_request*)(buffer.data+sizeof(Header));
			body.set_app_status(0);
			body.set_protocol_status(UNKNOWN_ROLE);

			transceiver->secure_write(sizeof(Header)+sizeof(End_request), id, kill_con);
			return true;
		}

		{
			message=messages.front();
			messages.pop();
		}

		if(!message.type)
		{
			const Header& header=*(Header*)message.data.get();
			const char* body=message.data.get()+sizeof(Header);
			switch(header.get_type())
			{
				case PARAMS:
				{
					if(state!=PARAMS) throw Exceptions::Record_out_of_order(id, state, PARAMS);
					if(header.get_content_length()==0) 
					{
						state=IN;
						break;
					}
					if(!session.fill(body, header.get_content_length())) throw Exceptions::Param(id);
					break;
				}

				case IN:
				{
					if(state!=IN) throw Exceptions::Record_out_of_order(id, state, IN);
					if(header.get_content_length()==0)
					{
						session.clear_post_buffer();
						session.parse_url(session.query_string, session.get);
						session.parse_url(session.cookies, session.cookie);
						state=OUT;
						if(response())
						{
							complete();
							return true;
						}
						break;
					}
					session.fill_posts(body, header.get_content_length());
					session.parse_url(session.query_string, session.get);
					session.parse_url(session.cookies, session.cookie);
					in_handler(header.get_content_length());
					break;
				}

				case ABORT_REQUEST:
				{
					return true;
				}

				default:
				{
					break;
				}
			}
		}
		else if(response())
		{
			complete();
			return true;
		}
	}
	catch(std::exception& e)
	{
		err << e.what();
		err.flush();
		complete();
		return true;
	}
	return false;
}

template void Fastcgipp::Request<char>::setloc(std::locale loc_);
template void Fastcgipp::Request<wchar_t>::setloc(std::locale loc_);
template<class char_t> void Fastcgipp::Request<char_t>::setloc(std::locale loc_)
{
	loc=make_locale<char_t>(loc_);
	out.imbue(loc);
	err.imbue(loc);
}

#include "utf8_codecvt_facet.cpp"
