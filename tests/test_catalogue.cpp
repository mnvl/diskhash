
#include <boost/test/unit_test.hpp>
#include <stdlib.h>
#include <vector>

#include "catalogue.h"

using namespace diskhash;

static void fill_catalogue(catalogue &cat)
{
	int n = 1000 + rand() % 1000;

	while(n-- > 0)
	{
		cat.find(unsigned(rand()) * unsigned(rand())) = rand();
	}
}

BOOST_AUTO_TEST_SUITE(catalogue_suite, * boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(split_operation)
{
	catalogue cat("test_map", 16);

	fill_catalogue(cat);

	std::vector<unsigned> vec;
	for(catalogue::const_iterator it = cat.begin(); it != cat.end(); it++)
	{
		vec.push_back(*it);
	}

	cat.split();

	for(std::vector<unsigned>::const_iterator it = vec.begin(); it != vec.end(); it++)
	{
		size_t n = it - vec.begin();
		BOOST_CHECK_EQUAL(cat.find(n << (cat.prefix_shift() + 1)), *it);
	}

	cat.close();
	unlink("test_map");
}

BOOST_AUTO_TEST_SUITE_END()
