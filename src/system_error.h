#pragma once

#if defined(__linux__)
	#include "linux/system_error.h"
#elif defined(__APPLE__)
	#include "macos/system_error.h"
#elif defined(_WIN32)
	#include "windows/system_error.h"
#else
	#error Your operating system is not supported, consider README for details
#endif