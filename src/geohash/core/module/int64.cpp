#include "geohash/int64.hpp"

#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <Eigen/Core>

namespace py = pybind11;

// Checking the value defining the precision of a geohash.
inline auto check_range(uint32_t precision) -> void {
  if (precision < 1 || precision > 64) {
    throw std::invalid_argument("precision must be within [1, 64]");
  }
}

void init_int64(py::module& m) {
  m.def(
       "error",
       [](const uint32_t& precision) -> py::tuple {
         check_range(precision);
         auto lat_lng_err = geohash::int64::error_with_precision(precision);
         return py::make_tuple(std::get<1>(lat_lng_err),
                               std::get<0>(lat_lng_err));
       },
       py::arg("precision"),
       "Returns the precision in longitude/latitude and degrees for the "
       "given precision")
      .def(
          "encode",
          [](const geohash::Point& point, uint32_t precision) -> uint64_t {
            check_range(precision);
            return geohash::int64::encode(point, precision);
          },
          py::arg("point"), py::arg("precision") = 64,
          "Encode a point into geohash with the given precision")
      .def(
          "encode",
          [](const Eigen::Ref<const Eigen::Matrix<geohash::Point, -1, 1>>&
                 points,
             const uint32_t precision) -> Eigen::Matrix<uint64_t, -1, 1> {
            check_range(precision);
            return geohash::int64::encode(points, precision);
          },
          py::arg("points"), py::arg("precision") = 64,
          "Encode points into geohash with the given precision")
      .def(
          "decode",
          [](const uint64_t hash, const uint32_t precision,
             const bool round) -> geohash::Point {
            check_range(precision);
            return geohash::int64::decode(hash, precision, round);
          },
          py::arg("hash"), py::arg("precision") = 64, py::arg("round") = false,
          "Decode a hash into a spherical equatorial point with the given "
          "precision. If round is true, the coordinates of the points will be "
          "rounded to the accuracy defined by the GeoHash.")
      .def(
          "decode",
          [](const Eigen::Ref<const Eigen::Matrix<uint64_t, -1, 1>>& hashs,
             const uint32_t precision,
             const bool round) -> Eigen::Matrix<geohash::Point, -1, 1> {
            check_range(precision);
            return geohash::int64::decode(hashs, precision, round);
          },
          py::arg("hashs"), py::arg("precision") = 64, py::arg("round") = false,
          "Decode hashs into a spherical equatorial points with the given bit "
          "depth. If round is true, the coordinates of the points will be "
          "rounded to the accuracy defined by the GeoHash.")
      .def(
          "bounding_box",
          [](const uint64_t hash, const uint32_t precision) -> geohash::Box {
            check_range(precision);
            return geohash::int64::bounding_box(hash, precision);
          },
          py::arg("hash"), py::arg("precision") = 64,
          "Returns the region encoded by the integer geohash with the "
          "specified precision.")
      .def(
          "bounding_boxes",
          [](const std::optional<geohash::Box>& box,
             const uint32_t precision) -> Eigen::Matrix<uint64_t, -1, 1> {
            check_range(precision);
            return geohash::int64::bounding_boxes(box, precision);
          },
          py::arg("box") = py::none(), py::arg("precision") = 5,
          "Returns the region encoded by the integer geohash with the "
          "specified precision.")
      .def(
          "neighbors",
          [](const uint64_t hash,
             const uint32_t precision) -> Eigen::Matrix<uint64_t, 8, 1> {
            check_range(precision);
            return geohash::int64::neighbors(hash, precision);
          },
          py::arg("box"), py::arg("precision") = 64,
          "Returns all neighbors hash clockwise from north around northwest "
          "at the given precision.")
      .def(
          "grid_properties",
          [](const geohash::Box& box,
             const uint32_t precision) -> std::tuple<uint64_t, size_t, size_t> {
            check_range(precision);
            return geohash::int64::grid_properties(box, precision);
          },
          py::arg("box") = py::none(), py::arg("precision") = 64,
          "Returns the property of the grid covering the given box: geohash of "
          "the minimum corner point, number of boxes in longitudes and "
          "latitudes.")
      .def(
          "where",
          // We want to return an associative dictionary between bytes and
          // tuples and not str and tuples.
          [](const Eigen::Ref<const Eigen::Matrix<uint64_t, -1, -1>>& hash)
              -> py::dict {
            auto result = py::dict();
            for (auto&& item : geohash::int64::where(hash)) {
              auto key = py::int_(item.first);
              result[key] = py::cast(item.second);
            }
            return result;
          },
          py::arg("hash"),
          "Returns the start and end indexes of the different GeoHash boxes.");
}