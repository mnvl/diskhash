
#include <assert.h>
#include <stdlib.h>

#include <iostream>
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

void test_catalogue()
{
	std::cout << "constructing cataloguee...\n";
	catalogue cat("test_map", 16);

	std::cout << "filling catalogue...\n";
	fill_catalogue(cat);

	std::cout << "copying it to vector...\n";
	std::vector<unsigned> vec;
	for(catalogue::const_iterator it = cat.begin(); it != cat.end(); it++)
	{
		vec.push_back(*it);
	}

	std::cout << "splitting catalogue...\n";
	cat.split();

	std::cout << "comparing catalogue with vector...\n";
	for(std::vector<unsigned>::const_iterator it = vec.begin(); it != vec.end(); it++)
	{
		size_t n = it - vec.begin();

		#if defined(VERBOSE)
		std::cout << n
				  << '\t' << *it
				  << '\t' << cat.find(n << cat.prefix_shift() + 1) << std::endl;
		#endif

		assert(cat.find(n << (cat.prefix_shift() + 1)) == *it);
	}

	cat.close();
	unlink("testmap");
}
