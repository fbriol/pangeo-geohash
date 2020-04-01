#include "geohash/store/leveldb.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace store = geohash::store;
namespace py = pybind11;

void init_store_leveldb(py::module& m) {
  py::class_<store::LevelDB, std::shared_ptr<store::LevelDB>>(
      m, "LevelDB", "A persistent ordered map from keys to values.")
      .def(py::init<std::string, bool, bool, const std::optional<size_t>&,
                    const std::optional<int>&, const std::optional<size_t>&,
                    const std::optional<size_t>&, const std::optional<int>&,
                    const std::optional<size_t>&, bool>(),
           py::arg("name"),
           py::arg("create_if_missing") = true,
           py::arg("error_if_exists") = false,
           py::arg("write_buffer_size") = py::none(),
           py::arg("max_open_files") = py::none(),
           py::arg("lru_cache_size") = py::none(),
           py::arg("block_size") = py::none(),
           py::arg("block_restart_interval") = py::none(),
           py::arg("max_file_size") = py::none(),
           py::arg("enable_compressio") = true, "Opening a database")
      .def(
          "__setitem__",
          [](store::LevelDB& self, const pybind11::bytes& key,
             const pybind11::object& value) {
            self.setitem(key, value, nullptr);
          },
          py::arg("key"), py::arg("value"))
      .def("__getitem__", &store::LevelDB::getitem, py::arg("key"))
      .def("__delitem__", &store::LevelDB::delitem, py::arg("key"))
      .def("__len__", &store::LevelDB::len)
      .def("__contains__", &store::LevelDB::contains, py::arg("key"))
      .def("clear", &store::LevelDB::clear,
           "Remove all entries from the database.")
      .def("keys", &store::LevelDB::keys,
           "Return a list containing all the keys from the database.")
      .def("update", &store::LevelDB::update, py::arg("map"),
           "Update the database with the key/value pairs from map, overwriting "
           "existing keys.")
      .def("extend", &store::LevelDB::extend, py::arg("map"),
           "Extend or create the database with the key/value pairs from map")
      .def("values", &store::LevelDB::values, py::arg("keys") = py::none(),
           "Read all values from the database for the keys provided");
}