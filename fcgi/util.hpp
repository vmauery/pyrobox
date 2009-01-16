#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <string>
#include <ostream>
#include <fstream>

//#define UNICODE_TEXT

#ifdef UNICODE_TEXT
#define wchar_type wchar_t
#define _S(A) L##A
#else
#define wchar_type char
#define _S(A) A
#endif

void log_close();
void _log(const char *file, char const *fn, int line, const std::basic_string<char> msg);
void _log(const char *file, char const *fn, int line, const std::basic_string<wchar_t> msg);
void _log(const char *file, char const *fn, int line, const char* msg);
void _log(const char *file, char const *fn, int line, std::ostream &msg);
#define msg(A) _log(__FILE__, __FUNCTION__, __LINE__, A)
#define here(A) msg("")

#endif
