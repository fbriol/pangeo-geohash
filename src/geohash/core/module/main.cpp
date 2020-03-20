#include "geohash/geometry.hpp"
#include "geohash/int64.hpp"
#include "geohash/string.hpp"
#include <Eigen/Core>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <sstream>
#include <string>

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
inline auto check_range(uint32_t precision, uint32_t max) -> void {
  if (precision < 1 || precision > max) {
    throw std::invalid_argument("precision must be within [1, " +
                                std::to_string(max) + "]");
  }
}

// Return a string containing a printable representation of a Point.
inline auto repr_point(const geohash::Point& point) -> std::string {
  auto ss = std::stringstream();
  ss << "Point(" << point.lng << ", " << point.lat << ")";
  return ss.str();
}

PYBIND11_MODULE(core, m) {
  py::class_<geohash::Point>(
      m, "Point",
      "Define a point using the geographic coordinate system, i.e. defined by "
      "two angles: longitude and latitude.")
      .def(py::init<>(), "Default constructor")
      .def(py::init<double, double>(), py::arg("lng"), py::arg("lat"),
           R"(Constructor to set two values
Args:
    lng (float): longitude in degrees
    lat (float): latitude in degrees
)")
      .def_property_readonly(
          "lng", [](const geohash::Point& self) -> double { return self.lng; },
          "Between -180 and 180")
      .def_property_readonly(
          "lat", [](const geohash::Point& self) -> double { return self.lat; },
          "Beween -90 and 90")
      .def("__repr__", [](const geohash::Point& self) -> std::string {
        return repr_point(self);
      });

  // The points can be defined in numpy arrays.
  PYBIND11_NUMPY_DTYPE(geohash::Point, lng, lat);

  py::class_<geohash::Box>(m, "Box", "A box made of two describing points")
      .def(py::init<geohash::Point, geohash::Point>(), py::arg("min_corner"),
           py::arg("max_corner"),
           R"(
Constructor taking the minimum corner point and the maximum corner point

Args:
    min_corner (geohash.Point): the minimum corner point
    max_corner (geohash.Point): the maximum corner point
)")
      .def_property_readonly(
          "min_corner",
          [](const geohash::Box& self) -> const geohash::Point& {
            return self.min_corner();
          },
          "The minimum corner point")
      .def_property_readonly(
          "max_corner",
          [](const geohash::Box& self) -> const geohash::Point& {
            return self.max_corner();
          },
          "the maximum corner point")
      .def("contains", &geohash::Box::contains, py::arg("point"),
           "Returns true if the geographic point is within the box")
      .def("__repr__", [](const geohash::Box& self) -> std::string {
        auto ss = std::stringstream();
        ss << "Box(" << repr_point(self.min_corner()) << ", "
           << repr_point(self.max_corner()) << ")";
        return ss.str();
      });

  auto int64 = m.def_submodule("int64");
  int64
      .def("error",
           [](const uint32_t& precision) -> py::tuple {
             check_range(precision, 64);
             auto lat_lng_err = geohash::int64::error_with_precision(precision);
             return py::make_tuple(std::get<1>(lat_lng_err),
                                   std::get<0>(lat_lng_err));
           },
           py::arg("precision"),
           "Returns the precision in longitude/latitude and degrees for the "
           "given precision")
      .def("encode",
           [](const geohash::Point& point, uint32_t precision) -> uint64_t {
             check_range(precision, 64);
             return geohash::int64::encode(point, precision);
           },
           py::arg("point"), py::arg("precision") = 64,
           "Encode a point into geohash with the given precision")
      .def("encode",
           [](const Eigen::Ref<const Eigen::Matrix<geohash::Point, -1, 1>>&
                  points,
              const uint32_t precision) -> Eigen::Matrix<uint64_t, -1, 1> {
             check_range(precision, 64);
             return geohash::int64::encode(points, precision);
           },
           py::arg("points"), py::arg("precision") = 64,
           "Encode points into geohash with the given precision")
      .def("decode",
           [](const uint64_t hash, const uint32_t precision,
              const bool round) -> geohash::Point {
             check_range(precision, 64);
             return geohash::int64::decode(hash, precision, round);
           },
           py::arg("hash"), py::arg("precision") = 64, py::arg("round") = false,
           "Decode a hash into a spherical equatorial point with the given "
           "precision. If round is true, the coordinates of the points will be "
           "rounded to the accuracy defined by the GeoHash.")
      .def("decode",
           [](const Eigen::Ref<const Eigen::Matrix<uint64_t, -1, 1>>& hashs,
              const uint32_t precision,
              const bool round) -> Eigen::Matrix<geohash::Point, -1, 1> {
             check_range(precision, 64);
             return geohash::int64::decode(hashs, precision, round);
           },
           py::arg("hashs"), py::arg("precision") = 64,
           py::arg("round") = false,
           "Decode hashs into a spherical equatorial points with the given bit "
           "depth. If round is true, the coordinates of the points will be "
           "rounded to the accuracy defined by the GeoHash.")
      .def("bounding_box",
           [](const uint64_t hash, const uint32_t precision) -> geohash::Box {
             check_range(precision, 64);
             return geohash::int64::bounding_box(hash, precision);
           },
           py::arg("hash"), py::arg("precision") = 64,
           "Returns the region encoded by the integer geohash with the "
           "specified precision.")
      .def("bounding_boxes",
           [](const std::optional<geohash::Box>& box,
              const uint32_t precision) -> Eigen::Matrix<uint64_t, -1, 1> {
             check_range(precision, 64);
             return geohash::int64::bounding_boxes(box, precision);
           },
           py::arg("box") = py::none(), py::arg("precision") = 5,
           "Returns the region encoded by the integer geohash with the "
           "specified precision.")
      .def("neighbors",
           [](const uint64_t hash,
              const uint32_t precision) -> Eigen::Matrix<uint64_t, 8, 1> {
             check_range(precision, 64);
             return geohash::int64::neighbors(hash, precision);
           },
           py::arg("box"), py::arg("precision") = 64,
           "Returns all neighbors hash clockwise from north around northwest "
           "at the given precision.")
      .def(
          "grid_properties",
          [](const geohash::Box& box,
             const uint32_t precision) -> std::tuple<uint64_t, size_t, size_t> {
            check_range(precision, 64);
            return geohash::int64::grid_properties(box, precision);
          },
          py::arg("box") = py::none(), py::arg("precision") = 64,
          "Returns the property of the grid covering the given box: geohash of "
          "the minimum corner point, number of boxes in longitudes and "
          "latitudes.");

  auto string = m.def_submodule("string");
  string
      .def("error",
           [](const uint32_t& precision) -> py::tuple {
             check_range(precision, 12);
             auto lat_lng_err =
                 geohash::int64::error_with_precision(precision * 5);
             return py::make_tuple(std::get<1>(lat_lng_err),
                                   std::get<0>(lat_lng_err));
           },
           py::arg("precision"),
           "Returns the precision in longitude/latitude and degrees for the "
           "given precision")
      .def("encode",
           [](const geohash::Point& point,
              const uint32_t precision) -> py::handle {
             auto result = std::array<char, 12>();
             check_range(precision, 12);
             geohash::string::encode(point, result.data(), precision);
             return PyBytes_FromStringAndSize(result.data(), precision);
           },
           py::arg("point"), py::arg("precision") = 12,
           "Encode a point into geohash with the given precision")
      .def("encode",
           [](const Eigen::Ref<const Eigen::Matrix<geohash::Point, -1, 1>>&
                  points,
              const uint32_t precision) -> pybind11::array {
             check_range(precision, 12);
             return geohash::string::encode(points, precision);
           },
           py::arg("points"), py::arg("precision") = 12,
           "Encode points into geohash with the given precision")
      .def("decode",
           [](const py::str& hash, const bool round) -> geohash::Point {
             auto buffer = parse_str(hash);
             return geohash::string::decode(buffer.data(), buffer.length(),
                                            round);
           },
           py::arg("hash"), py::arg("round") = false,
           "Decode a hash into a spherical equatorial point. If round is true, "
           "the coordinates of the points will be rounded to the accuracy "
           "defined by the GeoHash.")
      .def("decode",
           [](const pybind11::array& hashs,
              const bool round) -> Eigen::Matrix<geohash::Point, -1, 1> {
             return geohash::string::decode(hashs, round);
           },
           py::arg("hashs"), py::arg("round") = false,
           "Decode hashs into a spherical equatorial points. If round is true, "
           "the coordinates of the points will be rounded to the accuracy "
           "defined by the GeoHash.")
      .def("bounding_box",
           [](const py::str& hash) -> geohash::Box {
             auto buffer = parse_str(hash);
             return geohash::string::bounding_box(buffer.data(),
                                                  buffer.length());
           },
           py::arg("hash"), "Returns the region encoded by the geohash.")
      .def("bounding_boxes",
           [](const std::optional<geohash::Box>& box,
              const uint32_t precision) -> py::array {
             check_range(precision, 12);
             return geohash::string::bounding_boxes(box, precision);
           },
           py::arg("box") = py::none(), py::arg("precision") = 1,
           "Returns the region encoded by the geohash with the specified "
           "precision.")
      .def("neighbors",
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
            check_range(precision, 12);
            return geohash::int64::grid_properties(box, precision * 5);
          },
          py::arg("box") = py::none(), py::arg("precision") = 12,
          "Returns the property of the grid covering the given box: geohash of "
          "the minimum corner point, number of boxes in longitudes and "
          "latitudes.");
}