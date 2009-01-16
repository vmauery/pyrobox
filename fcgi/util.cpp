#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>

#include <util.hpp>

using namespace std;
using namespace boost;
static ofstream error;
static basic_ofstream<wchar_t> werror;
void _log(const char *file, char const *fn, int line, const std::basic_string<char> msg)
{
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << file << ":" << fn << ":" << line << ": " << msg << endl;
}

void _log(const char *file, char const *fn, int line, const std::basic_string<wchar_t> msg)
{
	if(!werror.is_open())
	{
		werror.open("/tmp/errlog", ios_base::out | ios_base::app);
		werror.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	werror << '[' << posix_time::second_clock::local_time() << "] " << file << ":" << fn << ":" << line << ": ";
	werror << msg << endl;
}

void _log(const char *file, char const *fn, int line, const char* msg)
{
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << file << ":" << fn << ":" << line << ": " << msg << endl;
}

void _log(const char *file, char const *fn, int line, std::ostream &msg)
{
	if(!error.is_open())
	{
		error.open("/tmp/errlog", ios_base::out | ios_base::app);
		error.imbue(locale(error.getloc(), new posix_time::time_facet()));
	}

	error << '[' << posix_time::second_clock::local_time() << "] " << file << ":" << fn << ":" << line << ": " << msg << endl;
}

void log_close() {
	if (werror.is_open()) {
		werror.close();
	}
	if (error.is_open()) {
		error.close();
	}
}

