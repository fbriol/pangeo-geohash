#include "geohash/store/pickle.hpp"
#include <pybind11/pybind11.h>

namespace store = geohash::store;
namespace py = pybind11;

void init_store_pickle(py::module& m) {
  py::class_<store::Pickle>(
      m, "Pickle", "Python object serialization with compression support.")
      .def(py::init(), "Default constructor")
      .def("dumps", &store::Pickle::dumps, py::arg("obj"),
           py::arg("compress") = 0,
           "Return the pickled representation of the object obj as a bytes "
           "object.")
      .def("loads", &store::Pickle::loads, py::arg("bytes_object"),
           "Return the reconstituted object hierarchy of the pickled "
           "representation bytes_object of an object.");
}