/*

Copyright (c) 2007 Manvel Avetisian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided	with the distribution.
    * Neither the name of Manvel Avetisian nor the names of his contributors may
	be used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

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
