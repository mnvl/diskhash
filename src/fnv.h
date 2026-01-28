#pragma once

#include <string_view>
#include "settings.h"

namespace diskhash {

inline hash_t fnv1a(std::string_view sv) {
    hash_t h = 2166136261u;
    for (unsigned char c : sv) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

}  // namespace diskhash
