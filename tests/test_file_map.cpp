
#include <boost/test/unit_test.hpp>

#include "file_map.h"

using namespace diskhash;

namespace {

void cleanup_files(const char *base)
{
	unlink(base);
}

struct basic_fixture {
	basic_fixture() { cleanup_files("test_map"); }
	~basic_fixture() { cleanup_files("test_map"); }
};

}

BOOST_AUTO_TEST_SUITE(file_map_suite, * boost::unit_test::disabled())

BOOST_FIXTURE_TEST_CASE(basic_operations, basic_fixture)
{
	static const size_t N = 4096;

	file_map fm("test_map", false, N * sizeof(long));

	long *start = (long *) fm.start();
	BOOST_REQUIRE(start != nullptr);

	for(size_t i = 0; i < N; i++)
	{
		start[i] = i;
	}

	for(size_t i = 0; i < N; i++)
	{
		BOOST_CHECK_EQUAL(start[i], static_cast<long>(i));
	}

	fm.close();
}

BOOST_AUTO_TEST_SUITE_END()
