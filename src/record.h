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


#if !defined(__DISKHASH__RECORD_H__)
#define __DISKHASH__RECORD_H__

namespace diskhash {

template<class T>
struct basic_record {
	T *data;
	size_t length;

	basic_record(T *d, size_t l):
		data(d),
		length(l)
	{
	}

	operator basic_record<T const>()
	{
		return basic_record<T const>(data, length);
	}
};

typedef basic_record<unsigned char> record;
typedef basic_record<unsigned char const> const_record;

inline record wrap(std::string &str)
{
	return record((unsigned char *) str.c_str(), str.size());
}

inline const_record wrap(std::string const &str)
{
	return const_record((unsigned char const *) str.c_str(), str.size());
}

template<class T>
inline record wrap(T &t)
{
	return record((unsigned char *) &t, sizeof(T));
}

template<class T>
inline const_record wrap(T const &t)
{
	return const_record((unsigned char const *) &t, sizeof(T));
}

// namespace diskhash
}

#endif //!defined(__DISKHASH__RECORD_H__)
