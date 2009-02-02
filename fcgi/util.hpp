#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <cctype>
#include <string>
#include <algorithm>
#include <ostream>
#include <fstream>
#include <logging.hpp>

//#define UNICODE_TEXT

#ifdef UNICODE_TEXT
#define wchar_type wchar_t
#define _S(A) L##A
#else
#define wchar_type char
#define _S(A) A
#endif

std::string check_url(std::string url);

std::string html_entities(const std::string& encode);

void lower_case(std::string &s);

// breaks str up on any char in delimiter
std::vector<std::string> explode(const std::string& delimiter, const std::string& str);

std::string ltrim(const std::string &s);

std::string rtrim(const std::string &s);

std::string trim(const std::string &s);

void read_config_file(const std::string& cfile);

const std::string& get_conf(const std::string& cid);

#endif
