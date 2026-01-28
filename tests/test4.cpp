
#include <string.h>
#include <iostream>
#include <stdexcept>

extern void test_vbe();
extern void test_container();
extern void test_container_remove();
extern void test_hash_map();
extern void test_hash_map_remove();
extern void test_hash_map_perf();

struct test_info {
	const char *tag;
	const char *name;
	void (*fp)();
	bool skip_by_default;
};

test_info tests[] = {
	{ "vbe", "variable byte encoding", test_vbe, false },
	{ "pc", "container", test_container, false },
	{ "pcr", "container remove", test_container_remove, false },
	{ "hv", "hash map validity", test_hash_map, false },
	{ "hvr", "hash map remove", test_hash_map_remove, false },
	{ "hp", "hash map performance", test_hash_map_perf, true }
};

void run_tests(const char *prefix)
{
	size_t prefix_len = 0;

	if(prefix)
	{
		prefix_len = strlen(prefix);
	}

	for(size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
	{
		if(prefix_len == 0 ? !tests[i].skip_by_default : strncmp(prefix, tests[i].tag, prefix_len) == 0)
		{
			std::cout << "*** running " << tests[i].name << " test\n";
			(*tests[i].fp)();
		}
	}
}

int main(int argc, char *argv[])
{
	try
	{
		if(argc == 1)
		{
			run_tests(0);
		}
		else
		{
			for(int i = 1; i < argc; i++)
			{
				run_tests(argv[i]);
			}
		}
	}
	catch(std::exception const &error)
	{
		std::cerr << error.what() << std::endl;
		return 1;
	}
	catch(...)
	{
		std::cerr << "unkown exception caught" << std::endl;
		return 1;
	}

	return 0;
}
