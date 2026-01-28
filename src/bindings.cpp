#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <string>
#include <stdexcept>

#include "settings.h"
#include "record.h"
#include "hash_map.h"

namespace nb = nanobind;

namespace {

// FNV-1a 32-bit hash
diskhash::hash_t fnv1a(const unsigned char *data, size_t len) {
    diskhash::hash_t h = 2166136261u;
    for (size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

class PyDiskHash {
public:
    PyDiskHash(const std::string &path, bool read_only)
        : map_(nullptr), path_(path), read_only_(read_only)
    {
        map_ = new diskhash::hash_map<>(path.c_str(), read_only);
    }

    ~PyDiskHash() {
        do_close();
    }

    nb::bytes get(nb::bytes key) {
        ensure_open();
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k.data, k.length);
        diskhash::record r = map_->find(h, k);
        if (!r.data)
            throw nb::key_error(std::string(key.c_str(), key.size()).c_str());
        return nb::bytes(reinterpret_cast<const char *>(r.data), r.length);
    }

    nb::object get_default(nb::bytes key, nb::object default_val) {
        ensure_open();
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k.data, k.length);
        diskhash::record r = map_->find(h, k);
        if (!r.data)
            return default_val;
        return nb::cast(nb::bytes(reinterpret_cast<const char *>(r.data), r.length));
    }

    void put(nb::bytes key, nb::bytes value) {
        ensure_open();
        if (read_only_)
            throw std::runtime_error("hash map is read-only");
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k.data, k.length);
        diskhash::record r = map_->find(h, k);
        if (r.data)
            throw nb::key_error(std::string(key.c_str(), key.size()).c_str());
        diskhash::const_record v(
            reinterpret_cast<const unsigned char *>(value.c_str()), value.size());
        map_->get(h, k, v);
    }

    bool contains(nb::bytes key) {
        ensure_open();
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k.data, k.length);
        diskhash::record r = map_->find(h, k);
        return r.data != nullptr;
    }

    void remove(nb::bytes key) {
        ensure_open();
        if (read_only_)
            throw std::runtime_error("hash map is read-only");
        auto k = make_key(key);
        diskhash::hash_t h = fnv1a(k.data, k.length);
        if (!map_->remove(h, k))
            throw nb::key_error(std::string(key.c_str(), key.size()).c_str());
    }

    size_t bytes_allocated() {
        ensure_open();
        return map_->bytes_allocated();
    }

    void close() {
        do_close();
    }

    PyDiskHash *enter() {
        return this;
    }

    void exit() {
        do_close();
    }

private:
    diskhash::hash_map<> *map_;
    std::string path_;
    bool read_only_;

    void ensure_open() {
        if (!map_)
            throw std::runtime_error("hash map is closed");
    }

    void do_close() {
        if (map_) {
            map_->close();
            delete map_;
            map_ = nullptr;
        }
    }

    static diskhash::const_record make_key(nb::bytes &b) {
        return diskhash::const_record(
            reinterpret_cast<const unsigned char *>(b.c_str()), b.size());
    }
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
        .def("bytes_allocated", &PyDiskHash::bytes_allocated);
}
