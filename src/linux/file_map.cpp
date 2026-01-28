#include "file_map.h"
#include <iostream>

diskhash::file_map::file_map(char const *filename, bool read_only, size_t length)
{
	if((fd_ = open(filename, read_only ? O_RDONLY : O_RDWR | O_CREAT, S_IREAD | S_IWRITE)) < 0)
	{
		throw system_error();
	}

	struct stat st;
	if(fstat(fd_, &st) != 0)
	{
		system_error last_error;
		::close(fd_);
		throw last_error;
	}

	if(length == 0)
	{
		length_ = st.st_size;
	}
	else
	{
		if(length > size_t(st.st_size))
		{
			if(lseek(fd_, length, SEEK_SET) < 0)
			{
				system_error last_error;
				::close(fd_);
				throw last_error;
			}

			if(write(fd_, "", 1) != 1)
			{
				system_error last_error;
				::close(fd_);
				throw last_error;
			}
		}

		length_ = length;
	}

	if((start_ = mmap(0, length_, read_only ? PROT_READ : PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_SHARED, fd_, 0)) == MAP_FAILED)
	{
		system_error last_error;
		::close(fd_);
		throw last_error;
	}
}

diskhash::file_map::~file_map()
{
	if(start_ || fd_ != -1)
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

void diskhash::file_map::resize(size_t length) {
	if(length == length_)
	{
		return;
	}

	if(length > length_)
	{
		if(lseek(fd_, length, SEEK_SET) < 0)
		{
			throw system_error();
		}

		if(write(fd_, "", 1) != 1)
		{
			throw system_error();
		}
	}

	void *start = mremap(start_, length_, length, MREMAP_MAYMOVE);

	if(start == MAP_FAILED)
	{
		throw system_error();
	}

	start_ = start;
	length_ = length;
}

void diskhash::file_map::close()
{
	if(start_)
	{
		if(munmap(start_, length_) < 0)
		{
			throw system_error();
		}

		start_ = 0;
	}

	if(fd_ != -1)
	{
		if(::close(fd_) < 0)
		{
			throw system_error();
		}

		fd_ = -1;
	}
}
