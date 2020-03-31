#include "geohash/store/pickle.hpp"
#include <string>
#include <zlib.h>

namespace geohash::store {

// Compress serialized data
static auto deflate(const pybind11::bytes& data, const int level)
    -> pybind11::bytes {
  char* source;
  ssize_t source_len;

  if (PyBytes_AsStringAndSize(data.ptr(), &source, &source_len) != 0) {
    throw std::runtime_error("unable to extract bytes contents");
  }

  // Allocation of the result buffer.
  auto dest_len = compressBound(source_len);
  auto result = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, dest_len + sizeof(int64_t)));
  if (result.ptr() == nullptr) {
    throw std::runtime_error("out of memory");
  }

  // Fill the buffer with 0
  auto dest = PyBytes_AsString(result.ptr());
  memset(dest, 0, dest_len + sizeof(unsigned long));

  {
    auto gil = pybind11::gil_scoped_release();
    // Compress the input data
    if (compress2(reinterpret_cast<Bytef*>(dest), &dest_len,
                  reinterpret_cast<Bytef*>(source), source_len,
                  level) != Z_OK) {
      throw std::runtime_error("out of memory");
    }
  }

  if (_PyBytes_Resize(&result.ptr(), dest_len + sizeof(unsigned long)) < 0) {
    throw pybind11::error_already_set();
  }
  // The returned bytes is suffixed by the size of the decompressed data
  memcpy(PyBytes_AsString(result.ptr()) + dest_len, &source_len,
         sizeof(unsigned long));
  return result;
}

// Uncompress serialized data
static auto inflate(const pybind11::bytes& data, unsigned long dest_len)
    -> pybind11::bytes {
  auto result = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, dest_len));
  if (result.ptr() == nullptr) {
    throw std::runtime_error("out of memory");
  }
  auto dest = PyBytes_AsString(result.ptr());
  {
    auto gil = pybind11::gil_scoped_release();
    // Uncompress data
    auto rc = uncompress(reinterpret_cast<Bytef*>(dest), &dest_len,
                         reinterpret_cast<Bytef*>(PyBytes_AsString(data.ptr())),
                         pybind11::len(data) - sizeof(unsigned long));
    if (rc == Z_MEM_ERROR) {
      throw std::runtime_error("out of memory");
    }
    if (rc != Z_OK) {
      throw std::runtime_error("data corrupted");
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
auto Pickle::dumps(const pybind11::object& obj, const int compress) const
    -> pybind11::bytes {
  if (compress < 0 || compress > 9) {
    throw std::invalid_argument("compress must be in [0, 9]");
  }
  pybind11::bytes data = dumps_(obj, -1);
  if (compress != 0) {
    return deflate(data, compress);
  }

  // The string containing the serialized object is suffixed with zeros to
  // indicate that no compression was performed.
  auto suffix = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, sizeof(unsigned long)));
  if (suffix.ptr() == nullptr) {
    throw std::runtime_error("out of memory");
  }
  memset(PyBytes_AsString(suffix.ptr()), 0, sizeof(unsigned long));
  PyBytes_Concat(&data.ptr(), suffix.ptr());
  if (data.ptr() == nullptr) {
    throw pybind11::error_already_set();
  }
  return data;
}

// ---------------------------------------------------------------------------
auto Pickle::loads(const pybind11::bytes& data) const -> pybind11::object {
  auto data_len = pybind11::len(data);
  if (data_len < sizeof(unsigned long)) {
    throw std::invalid_argument("invalid serialized data");
  }

  // Get the size of the inflated data.
  unsigned long dest_len;
  auto ptr = PyBytes_AsString(data.ptr());
  memcpy(&dest_len, ptr + data_len - sizeof(unsigned long),
         sizeof(unsigned long));

  // If the size is not zero, the string must be decompressed before
  // deserializing the object.
  if (dest_len == 0) {
    return loads_(data);
  }
  return loads_(inflate(data, dest_len));
}

}  // namespace geohash::store