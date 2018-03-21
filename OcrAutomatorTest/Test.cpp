//#include <BoostTestTargetConfig.h>
#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>
#include <string>
#include <boost/thread/thread.hpp>

boost::thread* th = nullptr;
BOOST_AUTO_TEST_CASE( my_test )
{
	std::string str = "12345";

    BOOST_CHECK( true );
}