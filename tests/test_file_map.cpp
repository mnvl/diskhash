
#include <assert.h>
#include <iostream>

#include "file_map.h"

using namespace diskhash;

void test_file_map()
{
	static const size_t N = 4096;

	std::cout << "creating test_map file..." << std::endl;
	file_map fm("test_map", false, N * sizeof(long));

	long *start = (long *) fm.start();
	std::cout << "mapped buffer start is " << start << std::endl;

	std::cout << "writing to map..." << std::endl;
	for(size_t i = 0; i < N; i++)
	{
		start[i] = i;
	}

	std::cout << "reading from map..." << std::endl;
	for(size_t i = 0; i < N; i++)
	{
		assert(start[i] == i);
	}

	fm.close();
	unlink("test_map");
}
