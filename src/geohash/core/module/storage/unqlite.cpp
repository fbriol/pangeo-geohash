#include "geohash/storage/unqlite.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace store = geohash::storage::unqlite;
namespace py = pybind11;

void init_store_unqlite(py::module& m) {
  py::register_exception<store::DatabaseError>(m, "DatabaseError",
                                               PyExc_RuntimeError);
  py::register_exception<store::ProgrammingError>(m, "ProgrammingError",
                                                  PyExc_RuntimeError);
  py::register_exception<store::OperationalError>(m, "OperationalError",
                                                  PyExc_RuntimeError);
  py::register_exception<store::LockError>(m, "LockError", PyExc_IOError);

  py::enum_<store::CompressionType>(m, "CompressionType")
      .value("none", store::kNoCompression, "No commpression")
      .value("snappy", store::kSnappyCompression,
             "Compress values with Snappy");

  py::class_<store::Database, std::shared_ptr<store::Database>>(
      m, "Database", "Key/Value store")
      .def(py::init<std::string, const std::optional<std::string>&,
                    store::CompressionType>(),
           py::arg("name"), py::arg("mode") = py::none(),
           py::arg("compression_type") = store::kSnappyCompression,
           R"(Opening a database
Args:
     name (str): path to the target database file. If name is ":mem:" then
          a private, in-memory database is created
     mode (str, optional): optional string that specifies the mode in which
          the database is opened. Default to `r` which means open for readind.
          The available mode are:

          ========= ========================================================
          Character Meaning
          ========= ========================================================
          ``'r'``   open for reading (default)
          ``'w'``   open for reading/writing. Create the database if needed
          ``'a'``   open for writing. Database file must be existing.
          ``'m'``   open in read-only memory view of the whole database
          ========= ========================================================

          Mode `m` works only in conjunction with `r` mode.
     compression_mode (CompressionMode, optional): Type of compression used
          to compress values stored into the database. Only has an effect for
          new data written in the database.
)")
      .def(py::pickle(
          [](const store::Database& self) -> py::tuple {
            return self.getstate();
          },
          [](const py::tuple& state) -> std::shared_ptr<store::Database> {
            return store::Database::setstate(state);
          }))
      .def("__setitem__", &store::Database::setitem, py::arg("key"),
           py::arg("value"))
      .def("__getitem__", &store::Database::getitem, py::arg("key"))
      .def("__delitem__", &store::Database::delitem, py::arg("key"))
      .def("__len__", &store::Database::len)
      .def("__contains__", &store::Database::contains, py::arg("key"))
      .def("error_log", &store::Database::error_log,
           "Reads the contents of the database error log")
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