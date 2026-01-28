#if !defined(system_error_h)
#define system_error_h

#include <errno.h>
#include <string.h>

#include <exception>

namespace diskhash {

class system_error: public std::exception {
public:
	system_error():
		error_number_(errno)
	{
	}

	char const *what() const throw() {
		return strerror(error_number_);
	}

private:
	int error_number_;
};

// namespace diskhash
}

#endif //!defined(system_error_h)
