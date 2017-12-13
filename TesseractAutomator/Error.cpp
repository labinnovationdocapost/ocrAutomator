#include "Error.h"
#include "Display.h"
#include "SlaveDisplay.h"
#include <cstdio>
#include <execinfo.h>
#include <stdexcept>
#include <exception>
#include <boost/stacktrace.hpp>

#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
//#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/attributes.hpp>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;

extern Display* display;
extern SlaveDisplay* sdisplay;
extern std::thread* th;



BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(tag_attr, "Tag", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Timestamp",
	boost::posix_time::ptime)
boost::log::sources::severity_logger<boost::log::trivial::severity_level> Log::CommonLogger;

void formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
	// Get the LineID attribute value and put it into the stream
	strm << logging::extract< unsigned int >("LineID", rec) << " | ";
	logging::value_ref< std::string > fullpath = logging::extract< std::string >("File", rec);
	strm << boost::filesystem::path(fullpath.get()).filename().string() << ":";
	strm << logging::extract< int >("Line", rec) << " | ";

	strm << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d_%H:%M:%S.%f");
	strm << " | ";
	// The same for the severity level.
	// The simplified syntax is possible if attribute keywords are used.
	strm << rec[logging::trivial::severity] << " | ";

	// Finally, put the record message to the stream
	strm << rec[expr::smessage];
}

void Log::InitLogger()
{
	logging::add_common_attributes();
	boost::shared_ptr< sinks::text_file_backend > backend = boost::make_shared< sinks::text_file_backend >(
		keywords::file_name = "TesseractAutomator_Log_%5N.log",
		keywords::rotation_size = 5 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
		);
	backend->auto_flush(true);
	
	//boost::shared_ptr< sinks::syslog_backend > backend = boost::make_shared< sinks::syslog_backend >();

	typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
	//typedef sinks::synchronous_sink< sinks::syslog_backend > sink_t;
	boost::shared_ptr<sink_t> sink(new sink_t(backend));

	sink->set_formatter(&formatter);
	sink->set_filter(severity >= boost::log::trivial::trace);

	logging::core::get()->add_sink(sink);
}

#if DISPLAY

void segfault_action(int sig, siginfo_t *info, void *secret)
{
	if (display != nullptr)
		display->terminated(true);
	if (sdisplay != nullptr)
		delete sdisplay;
	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;

	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Got signal: " << sig << ", faulty address is " << info->si_addr << ", from " << uc->uc_mcontext.gregs[REG_RIP];
	else
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "Got signal " << sig;

	std::cout << "Segfault happened view /var/log/TesseractAutomatorLog.log for more details" << std::endl << std::flush;

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *)uc->uc_mcontext.gregs[REG_RIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "[bt] Execution path:";
	for (i = 1; i < trace_size; ++i)
	{
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "[bt] " << messages[i];

		/* find first occurence of '(' or ' ' in message[i] and assume
		* everything before that is the file name. (Don't go beyond 0 though
		* (string terminator)*/
		size_t p = 0;
		while (messages[i][p] != '(' && messages[i][p] != ' '
			&& messages[i][p] != 0)
			++p;

		char syscom[512];
		sprintf(syscom, "addr2line %p -e %.*s", trace[i], p, messages[i]);
		//last parameter is the filename of the symbol
		FILE* ptr;
		char buf[BUFSIZ];

		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << syscom;
		if ((ptr = popen(syscom, "r")) != NULL) {
			while (fgets(buf, BUFSIZ, ptr) != NULL)
			{

				BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << buf;
			}
			pclose(ptr);
		}
	}
	if (th->joinable())
		th->join();
	exit(0);
}

void segint_action(int sig, siginfo_t *info, void *secret)
{
	if (display != nullptr)
		display->terminated(true);
	if (sdisplay != nullptr)
		delete sdisplay;

	BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "SIGINT received";

	/*if (th->joinable())
		th->join();*/
	delete display;
	exit(0);
}
void CatchAllErrorSignals()
{
	struct sigaction sa;
	struct sigaction sa2;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_action;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;

	memset(&sa2, 0, sizeof(struct sigaction));
	sigemptyset(&sa2.sa_mask);
	sa2.sa_sigaction = segint_action;
	sa2.sa_flags = SA_SIGINFO | SA_RESTART;

	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGINT, &sa2, nullptr);
}
std::string util_demangle(std::string to_demangle)
{
	int status = 0;
	char * buff = __cxxabiv1::__cxa_demangle(to_demangle.c_str(), NULL, NULL, &status);
	std::string demangled = buff;
	std::free(buff);
	return demangled;
}
typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> traced;

void terminated()
{
	if (display != nullptr)
		display->terminated(true);
	if (sdisplay != nullptr)
		delete sdisplay;

	if (th != nullptr && th->joinable())
	{
		th->join();
	}
	delete display;



	std::exception_ptr eptr = std::current_exception();
	try
	{
		std::rethrow_exception(eptr);
	}
	catch (boost::system::system_error &e)
	{
		std::cout << "Unhandled exception system_error : " << e.what() << std::endl;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << e.what();
	}
	catch (std::exception &e)
	{
		std::cout << "Unhandled exception : " << e.what() << std::endl;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << e.what();
	}
	catch (...)
	{
		std::string n = eptr.__cxa_exception_type()->name();
		std::cout << "Exception " << util_demangle(__cxxabiv1::__cxa_current_exception_type()->name()) << " happened\n" << "View /var/log/TesseractAutomatorException.log for more details" << std::endl << std::flush;
		std::cerr << "Unhandled exception " << n << std::endl;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "[Unhandled exception]: " << n;
	}

	exit(0);
}

void CatchAllExceptions()
{
	std::set_terminate(terminated);
}

void GenerateSIGSEGV()
{
	*(int*)0 = 0;
}
#endif
