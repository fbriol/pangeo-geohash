#include "geohash/storage/leveldb.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace store = geohash::storage::leveldb;
namespace py = pybind11;

void init_store_leveldb(py::module& m) {
  py::register_exception<store::CorruptionError>(m, "CorruptionError",
                                                 PyExc_RuntimeError);
  py::register_exception<store::InvalidArgumentError>(m, "InvalidArgumentError",
                                                      PyExc_ValueError);
  py::register_exception<store::IOError>(m, "IOError", PyExc_IOError);
  py::register_exception<store::NotFoundError>(m, "NotFoundError",
                                               PyExc_KeyError);
  py::register_exception<store::NotSupportedError>(m, "NotSupportedError",
                                                   PyExc_Exception);

  py::class_<store::Database, std::unique_ptr<store::Database>>(
      m, "Database", "A persistent ordered map from keys to values.")
      .def(py::init<const std::string&, bool, bool, bool,
                    const std::optional<size_t>&, const std::optional<int>&,
                    const std::optional<size_t>&, const std::optional<size_t>&,
                    const std::optional<int>&, const std::optional<size_t>&>(),
           py::arg("name"), py::arg("create_if_missing") = true,
           py::arg("error_if_exists") = false,
           py::arg("enable_compression") = true,
           py::arg("write_buffer_size") = py::none(),
           py::arg("max_open_files") = py::none(),
           py::arg("lru_cache_size") = py::none(),
           py::arg("block_size") = py::none(),
           py::arg("block_restart_interval") = py::none(),
           py::arg("max_file_size") = py::none(), "Opening a database")
      .def(
          "__setitem__",
          [](store::Database& self, const pybind11::bytes& key,
             const pybind11::object& value) {
            self.setitem(key, value, nullptr);
          },
          py::arg("key"), py::arg("value"))
      .def("__getitem__", &store::Database::getitem, py::arg("key"))
      .def("__delitem__", &store::Database::delitem, py::arg("key"))
      .def("__len__", &store::Database::len)
      .def("__contains__", &store::Database::contains, py::arg("key"))
      .def("clear", &store::Database::clear,
           "Remove all entries from the database.")
      .def("keys", &store::Database::keys,
           "Return a list containing all the keys from the database.")
      .def("update", &store::Database::update, py::arg("map"),
           "Update the database with the key/value pairs from map, overwriting "
           "existing keys.")
      .def("extend", &store::Database::extend, py::arg("map"),
           "Extend or create the database with the key/value pairs from map")
      .def("values", &store::Database::values, py::arg("keys") = py::none(),
           "Read all values from the database for the keys provided");

  py::class_<store::FileLock, std::unique_ptr<store::FileLock>>(
      m, "LockFile", "Handler to lock the specified file as leveldb does.")
      .def(py::init<const std::string>(), py::arg("name"));
}