#include "geohash/geometry.hpp"

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

#include "geohash/rtree.hpp"

namespace py = pybind11;

// Return a string containing a printable representation of a Point.
inline auto repr_point(const geohash::Point& point) -> std::string {
  auto ss = std::stringstream();
  ss << "Point(" << point.lng << ", " << point.lat << ")";
  return ss.str();
}

void init_geometry(py::module& m) {
  // geohash::Point
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

  // geohash::Box
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

  // The result obtained from the queries in the RTree
  PYBIND11_NUMPY_DTYPE(geohash::geometry::RTreeQueryResult, distance, index);

  py::class_<geohash::geometry::Projection>(m, "Projection",
                                            "Coordinate transformation")
      .def(py::init<std::string>(), py::arg("params"))
      .def(py::init<int>(), py::arg("code") = 4326);

  py::class_<geohash::geometry::RTree>(m, "RTree", "R*Tree")
      .def(py::init<std::optional<geohash::geometry::Projection>>(),
           py::arg("projection") = py::none())
      .def("clear", &geohash::geometry::RTree::clear)
      .def("__bool__", &geohash::geometry::RTree::empty)
      .def("__len__", &geohash::geometry::RTree::size)
      .def("packing", &geohash::geometry::RTree::packing)
      .def(
          "query",
          [](const geohash::geometry::RTree& self,
             const geohash::Point& coordinate, const uint32_t k,
             const bool within)
              -> Eigen::Matrix<geohash::geometry::RTreeQueryResult, -1, 1> {
            return self.query(coordinate, k, within);
          },
          py::arg("coordinate"), py::arg("k"), py::arg("within") = false)
      .def(
          "query",
          [](const geohash::geometry::RTree& self,
             const Eigen::Ref<const Eigen::Matrix<geohash::Point, -1, 1>>&
                 coordinates,
             const uint32_t k, bool within, size_t num_threads)
              -> Eigen::Matrix<geohash::geometry::RTreeQueryResult, -1, -1> {
            return self.query(coordinates, k, within, num_threads);
          },
          py::arg("coordinates"), py::arg("k"), py::arg("within") = false,
          py::arg("num_threads") = 0);
}
