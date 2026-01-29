#pragma once

#include <limits.h>
#include <stddef.h>

#if defined(_MSC_VER)
// warning C4996: '...' was declared deprecated
#pragma warning(disable: 4996)
#endif

namespace diskhash {

typedef unsigned hash_t;
size_t const HASH_BITS = sizeof(hash_t) * CHAR_BIT;

// make bucket size a multiple of standard page sizes for SSDs minus space for 3 size_t metadata fields
size_t const DEFAULT_BUCKET_SIZE = 4096 - 3 * sizeof(size_t);

// namespace diskhash
}
