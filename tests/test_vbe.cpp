
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

static void test_for_number(unsigned val)
{
	unsigned char buf[2 * sizeof(val)];
	unsigned char *p1 = diskhash::vbe::write(buf, val);

	unsigned val0;
	unsigned char *p2 = diskhash::vbe::read(buf, val0);

	assert(val0 == val);
	assert(p1 == p2);
	assert(p1 - buf == bytes_per_number(val));
}

void test_vbe()
{
	test_for_number(0);
	test_for_number(0x20);
	test_for_number(0x81);
	test_for_number(~0u);

	for(unsigned i = 0, n = 1; i < 1000*1000; i++)
	{
		test_for_number(n);
		n += rand();
	}
}
