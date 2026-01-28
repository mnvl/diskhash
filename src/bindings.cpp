#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>

#include "settings.h"
#include "hash_map.h"

namespace nb = nanobind;

namespace {

// FNV-1a 32-bit hash
diskhash::hash_t fnv1a(std::string_view sv) {
    diskhash::hash_t h = 2166136261u;
    for (unsigned char c : sv) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}

class PyDiskHash {
public:
    PyDiskHash(const std::string &path, bool read_only)
        : map_(std::make_unique<diskhash::hash_map<>>(path.c_str(), read_only)),
          path_(path), read_only_(read_only)
    {
    }

    nb::bytes get(nb::bytes key) {
        ensure_open();
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k);
        auto r = map_->find(h, k);
        if (!r)
            throw nb::key_error(std::string(key.c_str(), key.size()).c_str());
        return nb::bytes(r->data(), r->size());
    }

    nb::object get_default(nb::bytes key, nb::object default_val) {
        ensure_open();
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k);
        auto r = map_->find(h, k);
        if (!r)
            return default_val;
        return nb::cast(nb::bytes(r->data(), r->size()));
    }

    void put(nb::bytes key, nb::bytes value) {
        ensure_open();
        if (read_only_)
            throw std::runtime_error("hash map is read-only");
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k);
        auto r = map_->find(h, k);
        if (r)
            throw nb::key_error(std::string(key.c_str(), key.size()).c_str());
        std::string_view v(value.c_str(), value.size());
        map_->get(h, k, v);
    }

    bool contains(nb::bytes key) {
        ensure_open();
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k);
        return map_->find(h, k).has_value();
    }

    void remove(nb::bytes key) {
        ensure_open();
        if (read_only_)
            throw std::runtime_error("hash map is read-only");
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k);
        if (!map_->remove(h, k))
            throw nb::key_error(std::string(key.c_str(), key.size()).c_str());
    }

    size_t bytes_allocated() {
        ensure_open();
        return map_->bytes_allocated();
    }

    void close() {
        if (map_) {
            map_->close();
            map_.reset();
        }
    }

    PyDiskHash *enter() {
        return this;
    }

    void exit() {
        close();
    }

    diskhash::hash_map<> *map_ptr() {
        ensure_open();
        return map_.get();
    }

private:
    std::unique_ptr<diskhash::hash_map<>> map_;
    std::string path_;
    bool read_only_;

    void ensure_open() {
        if (!map_)
            throw std::runtime_error("hash map is closed");
    }

    static std::string_view make_key(nb::bytes &b) {
        return std::string_view(b.c_str(), b.size());
    }
};

class PyDiskHashIterator {
public:
    PyDiskHashIterator(diskhash::hash_map<> *map):
        it_(map->begin()), end_(map->end()) {}

    nb::tuple next()
    {
        if(it_ == end_)
            throw nb::stop_iteration();
        auto [key, value] = *it_;
        ++it_;
        return nb::make_tuple(nb::bytes(key.data(), key.size()),
                              nb::bytes(value.data(), value.size()));
    }

private:
    diskhash::hash_map<>::const_iterator it_, end_;
};

} // anonymous namespace

NB_MODULE(_diskhash, m) {
    nb::class_<PyDiskHash>(m, "DiskHash")
        .def(nb::init<const std::string &, bool>(),
             nb::arg("path"), nb::arg("read_only") = false)
        .def("get", &PyDiskHash::get_default,
             nb::arg("key"), nb::arg("default") = nb::none())
        .def("__getitem__", &PyDiskHash::get)
        .def("__setitem__", &PyDiskHash::put)
        .def("__contains__", &PyDiskHash::contains)
        .def("__delitem__", &PyDiskHash::remove)
        .def("remove", &PyDiskHash::remove)
        .def("__enter__", &PyDiskHash::enter, nb::rv_policy::reference)
        .def("__exit__", [](PyDiskHash &self, nb::args) { self.exit(); })
        .def("close", &PyDiskHash::close)
        .def("bytes_allocated", &PyDiskHash::bytes_allocated)
        .def("__iter__", [](PyDiskHash &self) {
            return PyDiskHashIterator(self.map_ptr());
        });

    nb::class_<PyDiskHashIterator>(m, "DiskHashIterator")
        .def("__iter__", [](PyDiskHashIterator &self) -> PyDiskHashIterator & { return self; })
        .def("__next__", &PyDiskHashIterator::next);
}
