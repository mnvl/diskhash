#pragma once

#include <windows.h>
#include "system_error.h"

namespace diskhash {

class file_map {
public:
	file_map(char const *file_name, bool read_only = false, size_t length = 0);
	~file_map();

	void *start() {
		return start_;
	}

	const void *start() const {
		return start_;
	}

	size_t length() const {
		return length_;
	}

	void resize(size_t new_length); 
	void close();

private:
	bool read_only_;
	HANDLE file_handle_;
	HANDLE mapping_handle_;
	PVOID start_;
	size_t length_;
};

// namespace diskhash
}
