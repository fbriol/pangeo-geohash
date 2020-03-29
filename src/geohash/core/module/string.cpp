#include "geohash/string.hpp"
#include "geohash/int64.hpp"
#include <Eigen/Core>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// Parsing of the string defining a GeoHash.
inline auto parse_str = [](const py::str& hash) -> auto {
  auto result = std::string(hash);
  if (result.length() < 1 || result.length() > 12) {
    throw std::invalid_argument("Geohash length must be within [1, 12]");
  }
  return result;
};

// Checking the value defining the precision of a geohash.
inline auto check_range(uint32_t precision) -> void {
  if (precision < 1 || precision > 12) {
    throw std::invalid_argument("precision must be within [1, 12]");
  }
}

void init_string(py::module& m) {
  m.def(
       "error",
       [](const uint32_t& precision) -> py::tuple {
         check_range(precision);
         auto lat_lng_err = geohash::int64::error_with_precision(precision * 5);
         return py::make_tuple(std::get<1>(lat_lng_err),
                               std::get<0>(lat_lng_err));
       },
       py::arg("precision"),
       "Returns the precision in longitude/latitude and degrees for the "
       "given precision")
      .def(
          "encode",
          [](const geohash::Point& point,
             const uint32_t precision) -> py::handle {
            auto result = std::array<char, 12>();
            check_range(precision);
            geohash::string::encode(point, result.data(), precision);
            return PyBytes_FromStringAndSize(result.data(), precision);
          },
          py::arg("point"), py::arg("precision") = 12,
          "Encode a point into geohash with the given precision")
      .def(
          "encode",
          [](const Eigen::Ref<const Eigen::Matrix<geohash::Point, -1, 1>>&
                 points,
             const uint32_t precision) -> pybind11::array {
            check_range(precision);
            return geohash::string::encode(points, precision);
          },
          py::arg("points"), py::arg("precision") = 12,
          "Encode points into geohash with the given precision")
      .def(
          "decode",
          [](const py::str& hash, const bool round) -> geohash::Point {
            auto buffer = parse_str(hash);
            return geohash::string::decode(buffer.data(), buffer.length(),
                                           round);
          },
          py::arg("hash"), py::arg("round") = false,
          "Decode a hash into a spherical equatorial point. If round is true, "
          "the coordinates of the points will be rounded to the accuracy "
          "defined by the GeoHash.")
      .def(
          "decode",
          [](const pybind11::array& hashs,
             const bool round) -> Eigen::Matrix<geohash::Point, -1, 1> {
            return geohash::string::decode(hashs, round);
          },
          py::arg("hashs"), py::arg("round") = false,
          "Decode hashs into a spherical equatorial points. If round is true, "
          "the coordinates of the points will be rounded to the accuracy "
          "defined by the GeoHash.")
      .def(
          "bounding_box",
          [](const py::str& hash) -> geohash::Box {
            auto buffer = parse_str(hash);
            return geohash::string::bounding_box(buffer.data(),
                                                 buffer.length());
          },
          py::arg("hash"), "Returns the region encoded by the geohash.")
      .def(
          "bounding_boxes",
          [](const std::optional<geohash::Box>& box,
             const uint32_t precision) -> py::array {
            check_range(precision);
            return geohash::string::bounding_boxes(box, precision);
          },
          py::arg("box") = py::none(), py::arg("precision") = 1,
          "Returns the region encoded by the geohash with the specified "
          "precision.")
      .def(
          "neighbors",
          [](const py::str& hash) {
            auto buffer = parse_str(hash);
            return geohash::string::neighbors(buffer.data(), buffer.length());
          },
          py::arg("box"),
          "Returns all neighbors hash clockwise from north around northwest")
      .def(
          "grid_properties",
          [](const geohash::Box& box,
             const uint32_t precision) -> std::tuple<uint64_t, size_t, size_t> {
            check_range(precision);
            return geohash::int64::grid_properties(box, precision * 5);
          },
          py::arg("box") = py::none(), py::arg("precision") = 12,
          "Returns the property of the grid covering the given box: geohash of "
          "the minimum corner point, number of boxes in longitudes and "
          "latitudes.");
}