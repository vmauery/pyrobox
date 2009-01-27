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

static inline std::string check_url(std::string url) {
	return url;
}

std::string html_entities(const std::string& encode);

static inline void lower_case(std::string &s) {
	transform(s.begin(), s.end(), s.begin(), (int(*)(int))tolower);
}

// breaks str up on any char in delimiter
std::vector<std::string> explode(const std::string& delimiter, const std::string& str);

static inline std::string ltrim(const std::string &s) {
	return s.substr(s.find_first_not_of(" \t\r\n"));
}

static inline std::string rtrim(const std::string &s) {
	return s.substr(0, s.find_last_not_of(" \t\r\n"));
}

static inline std::string trim(const std::string &s) {
	return s.substr(s.find_first_not_of(" \t\r\n"),
					s.find_last_not_of(" \t\r\n"));
}

#endif
