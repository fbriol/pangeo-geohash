#include "geohash/storage/pickle.hpp"
#include <pybind11/pybind11.h>

namespace store = geohash::storage;
namespace py = pybind11;

void init_store_pickle(py::module& m) {
  py::class_<store::Pickle>(m, "Pickle", "Python object serialization")
      .def(py::init(), "Default constructor")
      .def("dumps", &store::Pickle::dumps, py::arg("obj"),
           "Return the pickled representation of the object obj as a bytes "
           "object.")
      .def("loads", &store::Pickle::loads, py::arg("bytes_object"),
           "Return the reconstituted object hierarchy of the pickled "
           "representation bytes_object of an object.");
}