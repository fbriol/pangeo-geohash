#pragma once
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <cmath>
#include <list>
#include <tuple>

#include "geohash/math.hpp"

namespace geohash {

// Geographic point
struct Point {
  double lng{};
  double lat{};
};

// A box made of two describing points
class Box {
 public:
  // Default constructor
  Box() = default;

  // Constructor taking the minimum corner point and the maximum corner point
  Box(const Point& min_corner, const Point& max_corner)
      : min_corner_(min_corner), max_corner_(max_corner) {}

  // Returns the center of the box.
  [[nodiscard]] inline constexpr auto center() const -> Point {
    return {(min_corner_.lng + max_corner_.lng) * 0.5,
            (min_corner_.lat + max_corner_.lat) * 0.5};
  }

  // Returns the delta of the box in latitude and longitude.
  [[nodiscard]] inline auto delta(bool round) const
      -> std::tuple<double, double> {
    auto x = max_corner_.lng - min_corner_.lng;
    auto y = max_corner_.lat - min_corner_.lat;
    if (round) {
      x = Box::max_decimal_power(x);
      y = Box::max_decimal_power(y);
    }
    return std::make_tuple(x, y);
  }

  // Returns a point inside the box, making an effort to round to minimal
  // precision.
  [[nodiscard]] inline auto round() const -> Point {
    double x;
    double y;
    std::tie(x, y) = delta(true);
    return {std::ceil(min_corner_.lng / x) * x,
            std::ceil(min_corner_.lat / y) * y};
  }

  // Returns the box, or the two boxes on either side of the dateline if the
  // defined box wraps around the globe (i.e. the longitude of the min corner
  // is greater than the longitude of the max corner.)
  [[nodiscard]] auto split() const -> std::list<Box> {
    // box wraps around the globe ?
    if (min_corner_.lng > max_corner_.lng) {
      return {Box({min_corner_.lng, min_corner_.lat}, {180, max_corner_.lat}),
              Box({-180, min_corner_.lat}, {max_corner_.lng, max_corner_.lat})};
    }
    return {*this};
  }

  // Returns true if the geographic point is within the box
  [[nodiscard]] auto contains(const Point& point) const -> bool {
    // box wraps around the globe ?
    if (min_corner_.lng > max_corner_.lng) {
      for (const auto& item : split()) {
        if (!item.contains(point)) {
          return false;
        }
      }
      return true;
    }
    return (min_corner_.lat <= point.lat && point.lat <= max_corner_.lat &&
            min_corner_.lng <= point.lng && point.lng <= max_corner_.lng);
  }

  // Returns the minimum corner point
  [[nodiscard]] auto min_corner() const -> const Point& { return min_corner_; }

  // Returns the minimum corner point
  [[nodiscard]] auto min_corner() -> Point& { return min_corner_; }

  // Returns the maximum corner point
  [[nodiscard]] auto max_corner() const -> const Point& { return max_corner_; }

  // Returns the maximum corner point
  [[nodiscard]] auto max_corner() -> Point& { return max_corner_; }

 private:
  Point min_corner_{};
  Point max_corner_{};

  // Returns the maximum power of 10 from a number (x > 0)
  static auto max_decimal_power(const double x) -> double {
    auto m = static_cast<int32_t>(std::floor(std::log10(x)));
    return power10(m);
  }
};

}  // namespace geohash

BOOST_GEOMETRY_REGISTER_POINT_2D(
    geohash::Point, double,
    boost::geometry::cs::geographic<boost::geometry::degree>, lng, lat)

BOOST_GEOMETRY_REGISTER_BOX(geohash::Box, geohash::Point, min_corner(),
                            max_corner())

namespace geohash {

using Polygon = boost::geometry::model::polygon<Point>;

}  // namespace geohash
