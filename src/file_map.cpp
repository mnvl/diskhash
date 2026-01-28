#include "file_map.h"

file_map::file_map(char const *filename, bool read_only, size_t length)
{
	if((fd_ = open(filename, read_only ? O_RDONLY : O_RDWR | O_CREAT, S_IREAD | S_IWRITE)) < 0)
	{
		throw system_error(errno);
	}

	struct stat st;
	if(fstat(fd_, &st) != 0)
	{
		throw system_error(errno);
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
				throw system_error(errno);
			}

			if(write(fd_, "", 1) != 1)
			{
				throw system_error(errno);
			}
		}

		length_ = length;
	}

	if((start_ = mmap(0, length_, read_only ? PROT_READ : PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_SHARED, fd_, 0)) == MAP_FAILED)
	{
		throw system_error(errno);
	}
}

file_map::~file_map()
{
	if(munmap(start_, length_) < 0)
	{
		throw system_error(errno);
	}

	if(close(fd_) < 0)
	{
		throw system_error(errno);
	}
}

void file_map::resize(size_t length) {
	if(length == length_)
	{
		return;
	}

	if(length > length_)
	{
		if(lseek(fd_, length, SEEK_SET) < 0)
		{
			throw system_error(errno);
		}

		if(write(fd_, "", 1) != 1)
		{
			throw system_error(errno);
		}
	}

	void *start = mremap(start_, length_, length, MREMAP_MAYMOVE);

	if(start == MAP_FAILED)
	{
		throw system_error(errno);
	}

	start_ = start;
	length_ = length;
}
