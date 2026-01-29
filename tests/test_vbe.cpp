
#include <boost/test/unit_test.hpp>
#include <stdlib.h>

#include "vbe.h"

using namespace diskhash::vbe;

static size_t bytes_per_number(unsigned val)
{
	if(val < 128u) return 1;
	if(val < 128u*128u) return 2;
	if(val < 128u*128u*128u) return 3;
	if(val < 128u*128u*128u*128u) return 4;
	return 5;
}

static void check_number(unsigned val)
{
	unsigned char buf[2 * sizeof(val)];
	unsigned char *p1 = diskhash::vbe::write(buf, val);

	unsigned val0;
	unsigned char *p2 = diskhash::vbe::read(buf, val0);

	BOOST_CHECK_EQUAL(val0, val);
	BOOST_CHECK_EQUAL(p1, p2);
	BOOST_CHECK_EQUAL(static_cast<size_t>(p1 - buf), bytes_per_number(val));
}

BOOST_AUTO_TEST_SUITE(vbe)

BOOST_AUTO_TEST_CASE(specific_values)
{
	check_number(0);
	check_number(0x20);
	check_number(0x81);
	check_number(~0u);
}

BOOST_AUTO_TEST_CASE(random_values)
{
	for(unsigned i = 0, n = 1; i < 1000*1000; i++)
	{
		check_number(n);
		n += rand();
	}
}

BOOST_AUTO_TEST_CASE(length_boundaries)
{
	// Values at exact boundaries of each VBE byte count
	BOOST_CHECK_EQUAL(length(0u), 1u);
	BOOST_CHECK_EQUAL(length(1u), 1u);
	BOOST_CHECK_EQUAL(length(127u), 1u);
	BOOST_CHECK_EQUAL(length(128u), 2u);
	BOOST_CHECK_EQUAL(length(128u*128u - 1u), 2u);
	BOOST_CHECK_EQUAL(length(128u*128u), 3u);
	BOOST_CHECK_EQUAL(length(128u*128u*128u - 1u), 3u);
	BOOST_CHECK_EQUAL(length(128u*128u*128u), 4u);
	BOOST_CHECK_EQUAL(length(128u*128u*128u*128u - 1u), 4u);
	BOOST_CHECK_EQUAL(length(128u*128u*128u*128u), 5u);
	BOOST_CHECK_EQUAL(length(~0u), 5u);
}

BOOST_AUTO_TEST_CASE(length_matches_write)
{
	for(unsigned i = 0, n = 0; i < 1000*1000; i++)
	{
		BOOST_CHECK_EQUAL(length(n), bytes_per_number(n));
		n += rand();
	}
}

BOOST_AUTO_TEST_SUITE_END()
