#include <pybind11/pybind11.h>

namespace py = pybind11;

extern void init_geometry(py::module& m);
extern void init_int64(py::module& m);
extern void init_string(py::module& m);

PYBIND11_MODULE(core, m) {
  auto int64 = m.def_submodule("int64");
  auto string = m.def_submodule("string");

  init_geometry(m);
  init_int64(int64);
  init_string(string);
}