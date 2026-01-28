#if !defined(__DISKHASH__SYSTEM_ERROR_H__)
#define __DISKHASH__SYSTEM_ERROR_H__

#include <windows.h>
#include <stdexcept>

namespace diskhash {

class system_error: public std::exception {
private:
	DWORD error_code_;
	mutable LPCTSTR message_buffer_;

public:
	system_error() throw():
		error_code_(GetLastError()),
		message_buffer_(0)
	{
	}

	~system_error() throw()	{
		if(message_buffer_)
		{
			LocalFree((LPVOID) message_buffer_);
		}
	}

	char const *what() const throw() {
		if(!message_buffer_) {
			if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
				0, error_code_, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &message_buffer_, 0, 0))
			{
				return "system error: can't format error message";
			}
		}

		return message_buffer_;
	}
};

// namespace diskhash
}

#endif