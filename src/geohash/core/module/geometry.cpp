#include "geohash/geometry.hpp"

#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

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
      .def("wkt",
           [](const geohash::Point& self) -> std::string {
             auto ss = std::stringstream();
             ss << boost::geometry::wkt(self);
             return ss.str();
           })
      .def_static(
          "read_wkt",
          [](const std::string& wkt) -> geohash::Point {
            auto point = geohash::Point();
            boost::geometry::read_wkt(wkt, point);
            return point;
          },
          py::arg("wkt"), "Returns the WKT representation of the Point")
      .def("__repr__", [](const geohash::Point& self) -> std::string {
        auto ss = std::stringstream();
        ss << "geohash.Point" << boost::geometry::dsv(self);
        return ss.str();
      });

  // The points can be defined in numpy arrays.
  PYBIND11_NUMPY_DTYPE(geohash::Point, lng, lat);

  // geohash::Box
  py::class_<geohash::Box>(m, "Box", "A box made of two describing points")
      .def(py::init<>(),
           R"(
Default constructor, no initialization
)")
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
      .def_static(
          "whole_earth",
          []() -> geohash::Box {
            return geohash::Box({-180, -90}, {180, 90});
          },
          "Returns the box covering the whole earth.")
      .def("contains", &geohash::Box::contains, py::arg("point"),
           "Returns true if the geographic point is within the box")
      .def("wkt",
           [](const geohash::Box& self) -> std::string {
             auto ss = std::stringstream();
             ss << boost::geometry::wkt(self);
             return ss.str();
           })
      .def_static(
          "read_wkt",
          [](const std::string& wkt) -> geohash::Box {
            auto box = geohash::Box();
            boost::geometry::read_wkt(wkt, box);
            return box;
          },
          py::arg("wkt"), "Returns the WKT representation of the Box")
      .def("__repr__", [](const geohash::Box& self) -> std::string {
        auto ss = std::stringstream();
        ss << "geohash.Box" << boost::geometry::dsv(self);
        return ss.str();
      });

  py::class_<geohash::Polygon>(
      m, "Polygon",
      "The polygon contains an outer ring and zero or more inner rings")
      .def(
          py::init([](const Eigen::Ref<
                          const Eigen::Matrix<geohash::Point, -1, 1>>& outer,
                      std::optional<const py::list>& inners) {
            auto self = std::make_unique<geohash::Polygon>();
            for (auto ix = 0LL; ix < outer.size(); ++ix) {
              boost::geometry::append(self->outer(), outer[ix]);
            }
            if (inners.has_value()) {
              auto count = 0;
              self->inners().resize(inners->size());
              for (auto& item : *inners) {
                try {
                  auto inner = item.cast<
                      Eigen::Ref<const Eigen::Matrix<geohash::Point, -1, 1>>>();
                  for (auto ix = 0LL; ix < inner.size(); ++ix) {
                    boost::geometry::append(self->inners()[count], inner[ix]);
                  }
                } catch (py::cast_error) {
                  throw std::invalid_argument(
                      "outers must be a list of "
                      "numpy.ndarray[geohash.core.Point]");
                }
                ++count;
              }
            }
            return self;
          }),
          py::arg("outer"), py::arg("inners") = py::none(), R"(
Constructor filling the polygon

Args:
  outer (numpy.array): outer ring
  inners (list): list of inner rings
)")
      .def("__repr__",
           [](const geohash::Polygon& self) -> std::string {
             auto ss = std::stringstream();
             ss << "geohash.Polygon" << boost::geometry::dsv(self);
             return ss.str();
           })
      .def(
          "envelope",
          [](const geohash::Polygon& self) -> geohash::Box {
            auto box = geohash::Box();
            boost::geometry::envelope(self, box);
            return box;
          },
          "Calculates the envelope of this polygon.")
      .def("wkt",
           [](const geohash::Polygon& self) -> std::string {
             auto ss = std::stringstream();
             ss << boost::geometry::wkt(self);
             return ss.str();
           })
      .def_static(
          "read_wkt",
          [](const std::string& wkt) -> geohash::Polygon {
            auto polygon = geohash::Polygon();
            boost::geometry::read_wkt(wkt, polygon);
            return polygon;
          },
          py::arg("wkt"), "Returns the WKT representation of the Polygon");
}
