#include <pybind11/pybind11.h>

namespace py = pybind11;

extern void init_geometry(py::module& m);
extern void init_int64(py::module& m);
extern void init_store_pickle(py::module& m);
extern void init_store_unqlite(py::module& m);
extern void init_store_leveldb(py::module& m);
extern void init_string(py::module& m);

PYBIND11_MODULE(core, m) {
  auto int64 = m.def_submodule("int64", "GeoHash encoded as integer 64 bits");
  auto string = m.def_submodule("string", "GeoHash encoded as bytes");
  auto store = m.def_submodule("store", "Storage support");

  init_geometry(m);
  init_int64(int64);
  init_string(string);

  init_store_pickle(store);
  init_store_leveldb(store);
}