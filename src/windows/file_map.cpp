#include "file_map.h"
#include <iostream>

diskhash::file_map::file_map(char const *file_name, bool read_only, size_t length):
	read_only_(read_only),
	mapping_handle_(INVALID_HANDLE_VALUE),
	start_(0)
{
	file_handle_ = CreateFile(file_name, read_only ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE, 0,
		0, OPEN_EXISTING, 0, 0);

	if(file_handle_ == INVALID_HANDLE_VALUE)
	{
		if(read_only)
		{
			throw system_error();
		}

		file_handle_ = CreateFile(file_name, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);

		if(file_handle_ == INVALID_HANDLE_VALUE)
		{
			throw system_error();
		}
	}

	try
	{
		resize(length);
	}
	catch(...)
	{
		CloseHandle(file_handle_);
		throw;
	}
}

diskhash::file_map::~file_map()
{
	if(start_ || file_handle_ != INVALID_HANDLE_VALUE)
	{
		std::cerr << "diskhash: file_map destroyed without explicit close()\n";
		try
		{
			close();
		}
		catch(std::exception const &e)
		{
			std::cerr << "diskhash: error closing file_map in destructor: " << e.what() << "\n";
		}
		catch(...)
		{
			std::cerr << "diskhash: unknown error closing file_map in destructor\n";
		}
	}
}

void diskhash::file_map::resize(size_t new_length)
{
	HANDLE mapping_handle = CreateFileMapping(file_handle_, 0, read_only_ ? PAGE_READONLY : PAGE_READWRITE,
		0, (DWORD) new_length, 0);

	if(mapping_handle == INVALID_HANDLE_VALUE)
	{
		throw system_error();
	}

	PVOID start = MapViewOfFile(mapping_handle, read_only_ ? FILE_MAP_READ : FILE_MAP_WRITE,
		0, 0, new_length);

	if(!start)
	{
		system_error last_error;
		CloseHandle(mapping_handle);
		throw last_error;
	}

	if(start_)
	{
		UnmapViewOfFile(start_);
	}

	if(mapping_handle_)
	{
		CloseHandle(mapping_handle_);
	}

	mapping_handle_ = mapping_handle;

	start_ = start;
	length_ = new_length;
}

void diskhash::file_map::close()
{
	if(start_)
	{
		if(!UnmapViewOfFile(start_))
		{
			throw system_error();
		}

		start_ = 0;
	}

	if(mapping_handle_ != INVALID_HANDLE_VALUE)
	{
		if(!CloseHandle(mapping_handle_))
		{
			throw system_error();
		}

		mapping_handle_ = INVALID_HANDLE_VALUE;
	}

	if(file_handle_ != INVALID_HANDLE_VALUE)
	{
		if(!CloseHandle(file_handle_))
		{
			throw system_error();
		}

		file_handle_ = INVALID_HANDLE_VALUE;
	}
}