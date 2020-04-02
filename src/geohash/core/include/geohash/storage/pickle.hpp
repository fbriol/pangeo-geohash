#pragma once
#include <pybind11/pybind11.h>

namespace geohash::storage {

// Python object serialization with automatic compression
class Pickle {
 public:
  // Default constructor
  Pickle()
      : pickle_(pybind11::module::import("pickle")),
        dumps_(pickle_.attr("dumps")),
        loads_(pickle_.attr("loads")) {}

  // Return the pickled representation of the object obj as a bytes object
  [[nodiscard]] inline auto dumps(const pybind11::object& obj) const
      -> pybind11::bytes {
    return dumps_(obj, -1);
  }

  // Return the reconstituted object hierarchy of the pickled representation
  // bytes_object of an object.
  [[nodiscard]] inline auto loads(const pybind11::bytes& bytes_object) const
      -> pybind11::object {
    return loads_(bytes_object);
  }

 private:
  pybind11::module pickle_;
  pybind11::object dumps_;
  pybind11::object loads_;
};

}  // namespace geohash::storage