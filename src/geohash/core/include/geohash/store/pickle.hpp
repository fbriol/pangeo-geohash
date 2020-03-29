#pragma once
#include <pybind11/pybind11.h>

namespace geohash::store {

// Python object serialization with automatic compression
class Pickle {
 public:
  // Default constructor
  Pickle()
      : pickle_(pybind11::module::import("pickle")),
        dumps_(pickle_.attr("dumps")),
        loads_(pickle_.attr("loads")) {}

  // Return the pickled representation of the object obj as a bytes object
  auto dumps(const pybind11::object& obj, int compress) const
      -> pybind11::bytes;
  // Return the reconstituted object hierarchy of the pickled representation
  // bytes_object of an object.
  auto loads(const pybind11::bytes& data) const -> pybind11::object;

 private:
  pybind11::module pickle_;
  pybind11::object dumps_;
  pybind11::object loads_;
};

}  // namespace geohash::store