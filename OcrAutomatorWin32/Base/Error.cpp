#include "Base/Error.h"
#include "Master/Display.h"
#include "Slave/SlaveDisplay.h"
#include <cstdio>
#include <windows.h> 
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
#include "Slave/SlaveProcessingWorker.h"
#include "Master/MasterProcessingWorker.h"

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;

extern Display* display;
extern SlaveDisplay* sdisplay;
extern boost::thread* th;

extern Docapost::IA::Tesseract::MasterProcessingWorker* workerM;
extern Docapost::IA::Tesseract::SlaveProcessingWorker* workerS;


std::mutex Log::mFileExcludesMutex;
std::unique_ptr<std::ofstream> Log::mFileExcludes;

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
		keywords::file_name = "OcrAutomator_Log_%5N.log",
		keywords::rotation_size = 5 * 1024 * 1024,
		keywords::time_based_rotation = sinks::file::rotation_at_time_interval(boost::posix_time::seconds(1))
		);
	backend->auto_flush(true);
	backend->set_file_collector
	(
		sinks::file::make_collector
		(
			keywords::target = LOG_FOLDER
		)
	);
	backend->scan_for_files();
	//boost::shared_ptr< sinks::syslog_backend > backend = boost::make_shared< sinks::syslog_backend >();Tes

	typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
	//typedef sinks::synchronous_sink< sinks::syslog_backend > sink_t;
	boost::shared_ptr<sink_t> sink(new sink_t(backend));

	sink->set_formatter(&formatter);
	sink->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);

	logging::core::get()->add_sink(sink);
}

#if DISPLAY

void CatchAllErrorSignals()
{
}
std::string util_demangle(std::string to_demangle)
{
	const auto p = std::strchr(to_demangle.c_str(), ' ');
	std::string demangled = p ? p + 1 : to_demangle;
	return demangled;
}
typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace> traced;

void terminated()
{
	if (display != nullptr)
		display->terminated(true);
	if (sdisplay != nullptr)
		sdisplay->terminated(true);

	if (th != nullptr && th->joinable())
	{
		th->join();
	}

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
		std::cout << "Exception  happened\n" << "View logs for more details" << std::endl << std::flush;
		BOOST_LOG_WITH_LINE(Log::CommonLogger, boost::log::trivial::warning) << "[Unhandled exception]: ";
	}
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
