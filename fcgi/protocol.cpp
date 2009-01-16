//! \file protocol.cpp Defines Fast_cGI protocol
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


#include <protocol.hpp>

bool Fastcgipp::Protocol::process_param_header(const char* data, size_t data_size, const char*& name, size_t& name_size, const char*& value, size_t& value_size)
{
	if(*data>>7)
	{
		name_size=read_big_endian(*(uint32_t*)data) & 0x7fffffff;
		data+=sizeof(uint32_t);
	}
	else name_size=*data++;

	if(*data>>7)
	{
		value_size=read_big_endian(*(uint32_t*)data) & 0x7fffffff;
		data+=sizeof(uint32_t);
	}
	else value_size=*data++;
	name=data;
	value=name+name_size;
	if(name+name_size+value_size > data+data_size) return false;
	return true;
}

Fastcgipp::Protocol::Management_reply<14, 2, 8> Fastcgipp::Protocol::max_conns_reply("FCGI_MAX_CONNS", "10");
Fastcgipp::Protocol::Management_reply<13, 2, 1> Fastcgipp::Protocol::max_reqs_reply("FCGI_MAX_REQS", "50");
Fastcgipp::Protocol::Management_reply<15, 1, 8> Fastcgipp::Protocol::mpxs_conns_reply("FCGI_MPXS_CONNS", "1");

const char* Fastcgipp::Protocol::record_type_labels[] = { "INVALID", "BEGIN_REQUEST", "ABORT_REQUEST", "END_REQUEST", "PARAMS", "IN", "OUT", "ERR", "DATA", "GET_VALUES", "GET_VALUES_RESULT", "UNKNOWN_TYPE" };
