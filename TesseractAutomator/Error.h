#pragma once
#include <signal.h>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/thread.hpp>
namespace logging = boost::log;

namespace attrs = boost::log::attributes;

namespace src = boost::log::sources;


class Log
{
public:
	static boost::log::sources::severity_logger<boost::log::trivial::severity_level> CommonLogger;
	static void InitLogger();
};
//BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)

#define BOOST_LOG_WITH_LINE(log_, sv) BOOST_LOG_SEV( log_, sv) \
  << boost::log::add_value("Line", __LINE__)      \
  << boost::log::add_value("File", __FILE__)       \
  << boost::log::add_value("Function", BOOST_CURRENT_FUNCTION)

void segfault_action(int sig, siginfo_t *info, void *secret);

void CatchAllErrorSignals();
void CatchAllExceptions();

#ifdef TEST
void GenerateSIGSEGV();
#else
void GenerateSIGSEGV() __attribute__((error("DO NOT USE OUTSIDE OFF TEST CASE !!!")));
#endif // TEST
