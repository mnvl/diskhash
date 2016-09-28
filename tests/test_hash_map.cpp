
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <iostream>

#include "wrapped_hash_map.h"

using namespace diskhash;


struct hash
{
	unsigned operator ()(const std::string &k)
	{
		unsigned result = 0;

		for(size_t i = 0; i < k.size(); i++)
		{
			result = -13 * result  + 7 * (k[i] - 'a') + 111;
		}

		return result;
	}
};

std::string random_key()
{
	std::string k;

	for(size_t i = 0, n = rand() % 12 + 4; i < n; i++)
	{
		k += 'a' + rand() % ('z' - 'a');
	}

	return k;
}

void test_hash_map()
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test");
	std::map<std::string, unsigned> map2;

	for(int i = 0; i < 0x4000; i++)
	{
		std::string k = random_key();

		map1[k] = i;
		map2[k] = i;

		if((i & (i - 1)) == 0)
		{
			for(std::map<std::string, unsigned>::const_iterator it2 = map2.begin(); it2 != map2.end(); it2++)
			{
				assert(map1[it2->first] == it2->second);
			}
		}
	}

	for(std::map<std::string, unsigned>::const_iterator it2 = map2.begin(); it2 != map2.end(); it2++)
	{
		assert(map1[it2->first] == it2->second);
	}

	map1.close();

	unlink("testcat");
	unlink("testdat");
}

void test_hash_map_perf_inner(size_t records_count)
{
	size_t data_size = 0;

	#if defined(WIN32)
	LARGE_INTEGER c1;
	QueryPerformanceCounter(&c1);
	#else
	struct timespec c1;
	clock_gettime(CLOCK_REALTIME, &c1);
	#endif

	size_t bytes_allocated;

	{
		wrapped_hash_map<std::string, unsigned, hash> map1("test");

		for(unsigned i = 0; i < records_count; i++)
		{
			std::string str = random_key();
			map1[str] = i;

			data_size += str.size();
			data_size += sizeof(i);
		}

		bytes_allocated = map1.bytes_allocated() - records_count * sizeof(size_t);
	}

	unlink("testcat");
	unlink("testdat");

	#if defined(WIN32)
	LARGE_INTEGER c2;
	QueryPerformanceCounter(&c2);
	#else
	struct timespec c2;
	clock_gettime(CLOCK_REALTIME, &c2);
	#endif

	{
		std::map<std::string, unsigned> map2;

		for(unsigned i = 0; i < records_count; i++)
		{
			map2[random_key()] = i;
		}
	}

	#if defined(WIN32)
	LARGE_INTEGER c3;
	QueryPerformanceCounter(&c3);
	#else
	struct timespec c3;
	clock_gettime(CLOCK_REALTIME, &c3);
	#endif

	#if defined(WIN32)
	LARGE_INTEGER ticks_per_sec;
	QueryPerformanceFrequency((LARGE_INTEGER *) &ticks_per_sec);

	double delta1 = double(c2.QuadPart - c1.QuadPart) / double(ticks_per_sec.QuadPart),
		delta2 = (c3.QuadPart - c2.QuadPart) / double(ticks_per_sec.QuadPart);
	#else
	double delta1 = double(c2.tv_sec - c1.tv_sec) + double(c2.tv_nsec - c1.tv_nsec) / (1000 * 1000 * 1000),
		delta2 = double(c3.tv_sec - c2.tv_sec) + double(c3.tv_nsec - c2.tv_nsec + 1) / (1000 * 1000 * 1000);
	#endif

	printf("%.4f\t%.4f\t%.4f\t%lu\t%lu\t%lu\t%.2f\n", delta1, delta2, delta1 / delta2,
		records_count, data_size, bytes_allocated, double(bytes_allocated) / data_size);
}

void test_hash_map_perf()
{
	srand(time(0));

	for(size_t n = 4096; n <= 4 * 1024*1024; n <<= 1)
	{
		test_hash_map_perf_inner(n);
	}
}
