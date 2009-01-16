//! \file exceptions.cpp Defines fastcgi++ exceptions member functions
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

#include <exceptions.hpp>

#include <sstream>

Fastcgipp::Exceptions::Param::Param(Fastcgipp::Protocol::Full_id id_): Request(id_)
{
	std::stringstream sstr;
	sstr << "Error in parameter code conversion in request #" << id.fcgi_id << " of file descriptor #" << id.fd;
	msg=sstr.str();
}

Fastcgipp::Exceptions::Stream::Stream(Fastcgipp::Protocol::Full_id id_): Request(id_)
{
	std::stringstream sstr;
	sstr << "Error in output stream code conversion in request #" << id.fcgi_id << " of file descriptor #" << id.fd;
	msg=sstr.str();
}

Fastcgipp::Exceptions::Record_out_of_order::Record_out_of_order(Fastcgipp::Protocol::Full_id id_, Protocol::Record_type expected_record_, Protocol::Record_type recieved_record_)
: Request(id_), expected_record(expected_record_), recieved_record(recieved_record_)
{
	std::stringstream sstr;
	sstr << "Error: Parameter of type " << Protocol::record_type_labels[recieved_record] << " when type " << Protocol::record_type_labels[expected_record] << " was expected in request #" << id.fcgi_id << " of file descriptor #" << id.fd;
	msg=sstr.str();
}

Fastcgipp::Exceptions::Socket_write::Socket_write(int fd_, int erno_): Socket(fd_, erno_)
{
	std::stringstream sstr;
	sstr << "Error writing to socket #" << fd << " with errno=" << erno;
	msg=sstr.str();
}

Fastcgipp::Exceptions::Socket_read::Socket_read(int fd_, int erno_): Socket(fd_, erno_)
{
	std::stringstream sstr;
	sstr << "Error reading from socket #" << fd << " with errno=" << erno;
	msg=sstr.str();
}

Fastcgipp::Exceptions::Poll::Poll(int erno_): erno(erno_)
{
	std::stringstream sstr;
	sstr << "Error in poll with errno=" << erno;
	msg=sstr.str();
}
