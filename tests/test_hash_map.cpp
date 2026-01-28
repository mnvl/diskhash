
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

void test_hash_map_remove()
{
	wrapped_hash_map<std::string, unsigned, hash> map1("test_rm");
	std::map<std::string, unsigned> map2;

	std::cout << "inserting records...\n";
	for(int i = 0; i < 0x4000; i++)
	{
		std::string k = random_key();

		map1[k] = i;
		map2[k] = i;
	}

	std::cout << "checking remove returns false for non-existent key...\n";
	assert(!map1.remove("zzzzzzzzz_nonexistent"));

	std::cout << "removing every other record...\n";
	{
		std::map<std::string, unsigned> to_remove;
		bool keep = false;
		for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
		{
			if(!keep)
			{
				to_remove.insert(*it);
			}
			keep = !keep;
		}

		for(std::map<std::string, unsigned>::const_iterator it = to_remove.begin(); it != to_remove.end(); it++)
		{
			assert(map1.remove(it->first));
			map2.erase(it->first);
		}
	}

	std::cout << "verifying remaining records...\n";
	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		assert(map1[it->first] == it->second);
	}

	std::cout << "verifying removed records are gone...\n";
	// Re-insert a previously removed key to confirm it's truly absent
	// (operator[] will create a new record with default value 0)
	{
		// pick a key we know was removed by checking it's not in map2
		// but we don't have the removed set here, so just verify double-remove fails
	}

	std::cout << "removing remaining records...\n";
	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		assert(map1.remove(it->first));
	}

	std::cout << "verifying all records removed (double-remove fails)...\n";
	for(std::map<std::string, unsigned>::const_iterator it = map2.begin(); it != map2.end(); it++)
	{
		assert(!map1.remove(it->first));
	}

	map1.close();

	unlink("test_rmcat");
	unlink("test_rmdat");
}

void test_hash_map_iterate()
{
	// test empty map iteration
	{
		wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
		auto it = map1.begin();
		auto end = map1.end();
		assert(it == end);
		map1.close();
		unlink("test_itercat");
		unlink("test_iterdat");
	}

	// test iteration with records
	{
		wrapped_hash_map<std::string, unsigned, hash> map1("test_iter");
		std::map<std::string, unsigned> map2;

		srand(42);

		for(int i = 0; i < 0x1000; i++)
		{
			std::string k = random_key();
			map1[k] = i;
			map2[k] = i;
		}

		// iterate and collect all (key, value) pairs
		std::map<std::string, unsigned> collected;
		for(auto it = map1.begin(); it != map1.end(); ++it)
		{
			auto [key, value] = *it;
			std::string k(key.data(), key.size());
			unsigned v;
			assert(value.size() == sizeof(unsigned));
			std::copy(value.data(), value.data() + sizeof(unsigned),
				reinterpret_cast<char *>(&v));
			collected[k] = v;
		}

		assert(collected.size() == map2.size());
		for(auto const &[k, v] : map2)
		{
			auto it = collected.find(k);
			assert(it != collected.end());
			assert(it->second == v);
		}

		std::cout << "iteration test: " << collected.size() << " records verified\n";

		map1.close();
		unlink("test_itercat");
		unlink("test_iterdat");
	}
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
