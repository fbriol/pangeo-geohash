#include "geohash/storage/unqlite.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace store = geohash::storage::unqlite;
namespace py = pybind11;

void init_store_unqlite(py::module& m) {
  py::register_exception<store::DatabaseError>(m, "DatabaseError");
  py::register_exception<store::ProgrammingError>(m, "ProgrammingError");
  py::register_exception<store::OperationalError>(m, "OperationalError");
  py::register_exception<store::LockError>(m, "LockError");

  py::class_<store::Database, std::shared_ptr<store::Database>>(
      m, "Database", "Key/Value store")
      .def(py::init<const std::string&, bool, bool, bool>(), py::arg("name"),
           py::arg("create_if_missing") = true,
           py::arg("error_if_exists") = false,
           py::arg("enable_compression") = true, "Opening a database")
      .def("__setitem__", &store::Database::setitem, py::arg("key"),
           py::arg("value"))
      .def("__getitem__", &store::Database::getitem, py::arg("key"))
      .def("__delitem__", &store::Database::delitem, py::arg("key"))
      .def("__len__", &store::Database::len)
      .def("__contains__", &store::Database::contains, py::arg("key"))
      .def("error_log", &store::Database::error_log)
      .def("commit", &store::Database::commit,
           "Commit all changes to the database.")
      .def("rollback", &store::Database::rollback,
           "Rollback a write-transaction.")
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
}