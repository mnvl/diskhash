#if defined(__linux__)
	#include "linux/file_map.h"
#elif defined(__APPLE__)
	#include "macos/file_map.h"
#elif defined(_WIN32)
	#include "windows/file_map.h"
#else
	#error Your operating system is not supported, consider README for details
#endif
