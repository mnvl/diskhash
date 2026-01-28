#pragma once

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "hash_map.h"
#include "fnv.h"

namespace diskhash {

class sharded_hash_map {
public:
    sharded_hash_map(const std::string& base_path, size_t num_shards)
        : num_shards_(num_shards)
    {
        shards_.reserve(num_shards);
        for (size_t i = 0; i < num_shards; ++i) {
            std::string shard_path = base_path + "_shard" + std::to_string(i);
            shards_.push_back(std::make_unique<shard>(shard_path.c_str()));
        }
    }

    std::optional<std::string> get(const std::string& key) {
        size_t idx = shard_index(key);
        std::shared_lock lock(shards_[idx]->mutex);

        hash_t h = hash_key(key);
        auto result = shards_[idx]->map.find(h, key);
        if (result) {
            return std::string(*result);
        }
        return std::nullopt;
    }

    bool set(const std::string& key, const std::string& value) {
        size_t idx = shard_index(key);
        std::unique_lock lock(shards_[idx]->mutex);

        hash_t h = hash_key(key);
        // Check if key already exists
        if (shards_[idx]->map.find(h, key)) {
            return false;  // Key exists
        }

        // Insert the new key-value pair
        shards_[idx]->map.get(h, key, value);
        return true;
    }

    bool remove(const std::string& key) {
        size_t idx = shard_index(key);
        std::unique_lock lock(shards_[idx]->mutex);

        hash_t h = hash_key(key);
        return shards_[idx]->map.remove(h, key);
    }

    std::vector<std::string> keys() {
        std::vector<std::string> result;

        for (auto& shard : shards_) {
            std::shared_lock lock(shard->mutex);
            for (auto it = shard->map.begin(); it != shard->map.end(); ++it) {
                auto [k, v] = *it;
                result.emplace_back(k);
            }
        }

        return result;
    }

    void close() {
        for (auto& shard : shards_) {
            std::unique_lock lock(shard->mutex);
            shard->map.close();
        }
    }

private:
    struct shard {
        hash_map<> map;
        mutable std::shared_mutex mutex;

        explicit shard(const char* path) : map(path) {}
    };

    std::vector<std::unique_ptr<shard>> shards_;
    size_t num_shards_;

    size_t shard_index(const std::string& key) const {
        return fnv1a(key) % num_shards_;
    }

    static hash_t hash_key(const std::string& key) {
        return fnv1a(key);
    }
};

}  // namespace diskhash
