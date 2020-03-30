#include "geohash/store/unqlite.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace unqlite_ = geohash::store::unqlite;
namespace py = pybind11;

void init_store_unqlite(py::module& m) {
  py::register_exception<unqlite_::DatabaseError>(m, "DatabaseError");
  py::register_exception<unqlite_::ProgrammingError>(m, "ProgrammingError");
  py::register_exception<unqlite_::OperationalError>(m, "OperationalError");
  py::register_exception<unqlite_::LockError>(m, "LockError");

  py::class_<unqlite_::Options>(m, "Options",
                                "Options to control the behavior of a database")
      .def(py::init())
      .def_property("compression_level",
                    &unqlite_::Options::get_compression_level,
                    &unqlite_::Options::set_compression_level,
                    "Compression level for entries (0 no compression, 1 for "
                    "the fastest compression, 9 for the best compression).")
      .def_property("create_if_missing",
                    &unqlite_::Options::get_create_if_missing,
                    &unqlite_::Options::set_create_if_missing,
                    "If true, the database will be created if it is missing.");

  py::class_<unqlite_::Database, std::shared_ptr<unqlite_::Database>>(
      m, "Database", "Key/Value store")
      .def(py::init<std::string, std::optional<unqlite_::Options>>(),
           py::arg("filename"), py::arg("options") = py::none(),
           "Opening a database")
      .def(py::pickle([](const unqlite_::Database& self)
                          -> py::tuple { return self.getstate(); },
                      [](const py::tuple& state) -> std::shared_ptr<unqlite_::Database> {
                        return unqlite_::Database::setstate(state);
                      }))
      .def("__setitem__", &unqlite_::Database::setitem, py::arg("key"),
           py::arg("value"))
      .def("__getitem__", &unqlite_::Database::getitem, py::arg("key"))
      .def("__delitem__", &unqlite_::Database::delitem, py::arg("key"))
      .def("__len__", &unqlite_::Database::len)
      .def("__contains__", &unqlite_::Database::contains, py::arg("key"))
      .def("error_log", &unqlite_::Database::error_log)
      .def("commit", &unqlite_::Database::commit,
           "Commit all changes to the database.")
      .def("rollback", &unqlite_::Database::rollback,
           "Rollback a write-transaction.")
      .def("clear", &unqlite_::Database::clear,
           "Remove all entries from the database.")
      .def("keys", &unqlite_::Database::keys,
           "Return a list containing all the keys from the database.")
      .def("update", &unqlite_::Database::update, py::arg("map"),
           "Update the database with the key/value pairs from map, overwriting "
           "existing keys.")
      .def("extend", &unqlite_::Database::extend, py::arg("map"),
           "Extend or create the database with the key/value pairs from map")
      .def("values", &unqlite_::Database::values, py::arg("keys") = py::none(),
           "Read all values from the database for the keys provided")
      .def(
          "setdefault",
          [](const unqlite_::Database& self, const py::object& key,
             const py::object& default_) -> py::object {
            PyErr_SetNone(PyExc_NotImplementedError);
            throw pybind11::error_already_set();
          },
          py::arg("key"), py::arg("default") = py::none())
      .def(
          "pop",
          [](const unqlite_::Database& self, const py::object& key,
             const py::object& default_) -> py::object {
            PyErr_SetNone(PyExc_NotImplementedError);
            throw pybind11::error_already_set();
          },
          py::arg("key"), py::arg("default") = py::none())
      .def("popitem", [](const unqlite_::Database& self) -> py::tuple {
        PyErr_SetNone(PyExc_NotImplementedError);
        throw pybind11::error_already_set();
      });
}