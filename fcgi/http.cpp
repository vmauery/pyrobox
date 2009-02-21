//! \file http.cpp Defines elements of the HTTP protocol
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


#include <algorithm>
#include <map>
#include <functional>
#include <string>

#include <http.hpp>
#include <util.hpp>
#include <logging.hpp>
#include <debug.hpp>

void Fastcgipp::Http::Address::assign(const char* start, const char* end)
{
	data=0;
	for(int i=24; i>=0; i-=8)
	{
		char* point=(char*)memchr(start, '.', end-start);
		data|=atoi(start, end)<<i;
		if(!point || point+1>=end) break;
		start=point+1;
	}
}

template std::basic_ostream<char, std::char_traits<char> >& Fastcgipp::Http::operator<< <char, std::char_traits<char> >(std::basic_ostream<char, std::char_traits<char> >& os, const Address& address);
template<class char_t, class Traits> std::basic_ostream<char_t, Traits>& Fastcgipp::Http::operator<<(std::basic_ostream<char_t, Traits>& os, const Address& address)
{
	using namespace std;
	if(!os.good()) return os;
	
	try
	{
		typename basic_ostream<char_t, Traits>::sentry opfx(os);
		if(opfx)
		{
			streamsize field_width=os.width(0);
			char_t buffer[20];
			char_t* buf_ptr=buffer;
			locale loc(os.getloc(), new num_put<char_t, char_t*>);

			for(uint32_t mask=0xff000000, shift=24; mask!=0; mask>>=8, shift-=8)
			{
				buf_ptr=use_facet<num_put<char_t, char_t*> >(loc).put(buf_ptr, os, os.fill(), static_cast<long unsigned int>((address.data&mask)>>shift));
				*buf_ptr++=os.widen('.');
			}
			--buf_ptr;

			char_t* ptr=buffer;
			ostreambuf_iterator<char_t,Traits> sink(os);
			if(os.flags() & ios_base::left)
				for(int i=max(field_width, buf_ptr-buffer); i>0; i--)
				{
					if(ptr!=buf_ptr) *sink++=*ptr++;
					else *sink++=os.fill();
				}
			else
				for(int i=field_width-(buf_ptr-buffer); ptr!=buf_ptr;)
				{
					if(i>0) { *sink++=os.fill(); --i; }
					else *sink++=*ptr++;
				}

			if(sink.failed()) os.setstate(ios_base::failbit);
		}
	}
	catch(bad_alloc&)
	{
		ios_base::iostate exception_mask = os.exceptions();
		os.exceptions(ios_base::goodbit);
		os.setstate(ios_base::badbit);
		os.exceptions(exception_mask);
		if(exception_mask & ios_base::badbit) throw;
	}
	catch(...)
	{
		ios_base::iostate exception_mask = os.exceptions();
		os.exceptions(ios_base::goodbit);
		os.setstate(ios_base::failbit);
		os.exceptions(exception_mask);
		if(exception_mask & ios_base::failbit) throw;
	}
	return os;
}

template std::basic_istream<char, std::char_traits<char> >& Fastcgipp::Http::operator>> <char, std::char_traits<char> >(std::basic_istream<char, std::char_traits<char> >& is, Address& address);
template<class char_t, class Traits> std::basic_istream<char_t, Traits>& Fastcgipp::Http::operator>>(std::basic_istream<char_t, Traits>& is, Address& address)
{
	using namespace std;
	if(!is.good()) return is;

	ios_base::iostate err = ios::goodbit;
	try
	{
		typename basic_istream<char_t, Traits>::sentry ipfx(is);
		if(ipfx)
		{
			uint32_t data=0;
			istreambuf_iterator<char_t, Traits> it(is);
			for(int i=24; i>=0; i-=8, ++it)
			{
				uint32_t value;
				use_facet<num_get<char_t, istreambuf_iterator<char_t, Traits> > >(is.getloc()).get(it, istreambuf_iterator<char_t, Traits>(), is, err, value);
				data|=value<<i;
				if(i && *it!=is.widen('.')) err = ios::failbit;
			}
			if(err == ios::goodbit) address=data;
			else is.setstate(err);
		}
	}
	catch(bad_alloc&)
	{
		ios_base::iostate exception_mask = is.exceptions();
		is.exceptions(ios_base::goodbit);
		is.setstate(ios_base::badbit);
		is.exceptions(exception_mask);
		if(exception_mask & ios_base::badbit) throw;
	}
	catch(...)
	{
		ios_base::iostate exception_mask = is.exceptions();
		is.exceptions(ios_base::goodbit);
		is.setstate(ios_base::failbit);
		is.exceptions(exception_mask);
		if(exception_mask & ios_base::failbit) throw;
	}

	return is;
}

template bool Fastcgipp::Http::parse_xml_value<char>(const char* const name, const char* start, const char* end, std::basic_string<char>& string);
template<class char_t> bool Fastcgipp::Http::parse_xml_value(const char* const name, const char* start, const char* end, std::basic_string<char_t>& string)
{
	using namespace std;

	size_t search_string_size=strlen(name)+2;
	char* search_string=new char[search_string_size+1];
	memcpy(search_string, name, search_string_size-2);
	*(search_string+search_string_size-2)='=';
	*(search_string+search_string_size-1)='"';
	*(search_string+search_string_size)='\0';

	const char* value_start=0;

	for(; start<=end-search_string_size; ++start)
	{
		if(value_start && *start=='"') break;
		if(!memcmp(search_string, start, search_string_size))
		{
			value_start=start+search_string_size;
			start+=search_string_size-1;
		}
	}

	delete [] search_string;

	if(!value_start)
		return false;

	if(start-value_start) char_to_string(value_start, start-value_start, string);
	return true;
}

int Fastcgipp::Http::atoi(const char* start, const char* end)
{
	bool neg=false;
	if(*start=='-')
	{
		neg=false;
		++start;
	}
	int result=0;
	for(; 0x30 <= *start && *start <= 0x39 && start<end; ++start)
		result=result*10+(*start&0x0f);

	return neg?-result:result;
}

template int Fastcgipp::Http::percent_escaped_to_real_bytes(const char* source, char* dest, size_t size);
template<class char_t> int Fastcgipp::Http::percent_escaped_to_real_bytes(const char_t* source, char_t* dest, size_t size)
{
	int i=0;
	char_t* start=dest;
	if (!size) {
		*start = 0;
		return 0;
	}
	while(1)
	{
		if(*source=='%')
		{
			*dest=0;
			for(int shift=4; shift>=0; shift-=4)
			{
				if(++i==size) break;
				++source;
				if((*source|0x20) >= 'a' && (*source|0x20) <= 'f')
					*dest|=(*source|0x20)-0x57<<shift;
				else if(*source >= '0' && *source <= '9')
					*dest|=(*source&0x0f)<<shift;
			}
			++source;
			++dest;
			if(++i==size) break;
		}
		else if (*source == '+') {
			*dest++ = ' ';
			source++;
			if(++i==size) break;
		}
		else
		{
			*dest++=*source++;
			if(++i==size) break;
		}
	}
	*dest = 0;
	return dest-start;
}

template bool Fastcgipp::Http::Session<char>::fill(const char* data, size_t size);
template<class char_t> bool Fastcgipp::Http::Session<char_t>::fill(const char* data, size_t size)
{
	using namespace std;
	using namespace boost;
	
	bool status=true;

	while(size)
	{
		size_t name_size;
		size_t value_size;
		const char* name;
		const char* value;
		basic_string<char_t> svalue, sname;
		if(!Protocol::process_param_header(data, size, name, name_size, value, value_size))
			return false;

		size-=value-data+value_size;
		data=value+value_size;
		
		if(name_size==12 && !memcmp(name, "HTTP_REFERER", 12) && value_size)
		{
			char *buffer = new char[value_size+1];
			status=char_to_string(buffer, percent_escaped_to_real_bytes(value, buffer, value_size), svalue);
			delete [] buffer;
		}
		else if(name_size==12 && !memcmp(name, "CONTENT_TYPE", 12))
		{
			info(name << ": " << value);
			const char* end=(char*)memchr(value, ';', value_size);
			boundary_size = 0;
			status=char_to_string(value, end?end-value:value_size, svalue);
			if(end)
			{
				const char* start=end;
				// 9 is the length of "boundary="
				while (start-value < value_size-9) {
					if (memcmp(start, "boundary=", 9) == 0) {
						start += 9;
						if ((end = strpbrk(start, "; \n\r\t")) == NULL)
							boundary_size=value+value_size-(start);
						else
							boundary_size=end-(start);
						boundary.reset(new char[boundary_size]);
						memcpy(boundary.get(), start, boundary_size);
						break;
					}
					start++;
				}
			}
			log_dump(boundary.get(), boundary_size);
		}
		else if(name_size==12 && !memcmp(name, "QUERY_STRING", 12) && value_size)
		{
			char *buffer = new char[value_size+1];
			status=char_to_string(buffer, percent_escaped_to_real_bytes(value, buffer, value_size), svalue);
			query_string = svalue;
			delete [] buffer;
		}
		else if(name_size==22 && !memcmp(name, "HTTP_IF_MODIFIED_SINCE", 22))
		{
			basic_stringstream<char_t> date_stream;
			posix_time::time_input_facet tif("%a, %d %b %Y %H:%M:%S GMT");
			date_stream.write((char_t *)value, value_size);
			date_stream.imbue(locale(locale::classic(), &tif));
			date_stream >> svalue;
		} else {
			status=char_to_string(value, value_size, svalue);
		}
		status=char_to_string(name, name_size, sname);
		headers[sname] = svalue;
	}
	return status;
}

template void Fastcgipp::Http::Session<char>::parse_url(const std::basic_string<char>&, std::map<std::basic_string<char>, std::basic_string<char> >&);
template <class char_t> void Fastcgipp::Http::Session<char_t>::parse_url(const std::basic_string<char_t> &url, std::map<std::basic_string<char_t>, std::basic_string<char_t> > &query)
{
	// parse the query_string into a map
	int start = 0, end;
	std::basic_string<char_t> name;
	while (1) {
		char_t delim[] = { 0x26, 0x3d, 0x00 };
		char_t sep = 0x26;
		end = url.find_first_of(delim, start);
		if (end == std::basic_string<char_t>::npos) {
			if (start <= url.length()) {
				if (name.length() == 0) {
					name = url.substr(start);
					if (name.length() > 0)
						query[name] = std::basic_string<char_t>();
				} else {
					query[name] = url.substr(start);
				}
			}
			break;
		} else if (url[end] == sep) {
			if (name.length() == 0) {
				name = url.substr(start, end-start);
				query[name] = std::basic_string<char_t>();
			} else {
				query[name] = url.substr(start, end-start);
			}
			name = std::basic_string<char_t>();
		} else { // (url[end] == _S('='))
			name = url.substr(start, end-start);
		}
		start = end+1;
	}
}

template void Fastcgipp::Http::Session<char>::fill_posts(const char* data, size_t size);
template<class char_t> void Fastcgipp::Http::Session<char_t>::fill_posts(const char* data, size_t size)
{
	using namespace std;
	info("boundary_size = " << boundary_size);
	log_dump(data, size);
	if (boundary_size == 0) {
		typename strmap::iterator iter;
		strmap p;
		char_t *buffer = new char_t[size];
		basic_string<char_t> url, value;
		char_to_string(data, size, url);
		parse_url(url, p);
		for (iter=p.begin(); iter!=p.end(); iter++) {
			size_t val_len = iter->second.length();
			char_t *unescaped = buffer;
			percent_escaped_to_real_bytes(iter->second.c_str(), unescaped, val_len);
			value.assign(unescaped);
			post[iter->first] = value;
		}
		delete[] buffer;
		return;
	}
	while(1) {
		size_t buffer_size=post_buffer_size+size;
		char* buffer=new char[buffer_size];
		if(post_buffer_size) memcpy(buffer, post_buffer.get(), post_buffer_size);
		memcpy(buffer+post_buffer_size, data, size);
		post_buffer.reset(buffer);
		post_buffer_size=buffer_size;

		const char* end=0;
		for(const char* i=buffer+boundary_size; i<buffer+buffer_size-boundary_size; ++i) {
			if(!memcmp(i, boundary.get(), boundary_size)) {
				end=i;
				break;
			}
		}
		
		if(!end)
			return;

		end-=4;
		const char* start=buffer+boundary_size+2;
		const char* body_start=start;
		for(; body_start<=end-4; ++body_start) {
			if(!memcmp(body_start, "\r\n\r\n", 4)) break;
		}
		body_start+=4;
		basic_string<char_t> name, value;

		if(parse_xml_value("name", start, body_start, name)) {
			if(parse_xml_value("filename", start, body_start, value)) {
				size_t size=end-body_start;
				if(size) {
					basic_string<char_t> mime;
					basic_string<char_t> headers;
					char_to_string(start, body_start-start, headers);
					size_t ct_off = headers.find("Content-Type: ");
					if (ct_off != basic_string<char_t>::npos) {
						mime = trim(headers.substr(ct_off+13));
					}
					boost::shared_array<char> data;
					data.reset(new char[size]);
					memcpy(data.get(), body_start, size);
					files[name] = upload_file<char_t>::create(value, mime, data, size);
				}
			} else {
				basic_string<char_t>& value=post[name];
				char_to_string(body_start, end-body_start, value);
			}
		}
		buffer_size=buffer_size-(end-buffer+2);
		if(!buffer_size) {
			post_buffer.reset();
			return;
		}
		char *new_buffer = new char[buffer_size];
		memcpy(new_buffer, end+2, buffer_size);
		post_buffer.reset(new_buffer);
		post_buffer_size=buffer_size;
		size=0;
	}
}


