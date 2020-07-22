#pragma once
#include <Eigen/Core>
#include <map>
#include <optional>
#include <tuple>

#include "geohash/geometry.hpp"
#include "geohash/math.hpp"

namespace geohash::int64 {

// Returns the precision in longitude/latitude and degrees for the given
// precision
[[nodiscard]] inline auto constexpr error_with_precision(
    const uint32_t precision) -> std::tuple<double, double> {
  auto lat_bits = static_cast<int32_t>(precision >> 1U);
  auto lng_bits = static_cast<int32_t>(precision - lat_bits);

  return std::make_tuple(360 * power2(-lng_bits), 180 * power2(-lat_bits));
}

// Encode a point into geohash with the given precision
[[nodiscard]] auto encode(const Point& point, uint32_t precision) -> uint64_t;

// Encode points into geohash with the given precision
[[nodiscard]] inline auto encode(
    const Eigen::Ref<const Eigen::Matrix<Point, -1, 1>>& points,
    uint32_t precision) -> Eigen::Matrix<uint64_t, -1, 1> {
  auto result = Eigen::Matrix<uint64_t, -1, 1>(points.size());
  for (Eigen::Index ix = 0; ix < points.size(); ++ix) {
    result(ix) = encode(points(ix), precision);
  }
  return result;
}

// Returns the region encoded by the integer geohash with the specified
// precision.
[[nodiscard]] auto bounding_box(uint64_t hash, uint32_t precision) -> Box;

// Decode a hash into a spherical equatorial point with the given precision.
// If round is true, the coordinates of the points will be rounded to the
// accuracy defined by the GeoHash.
[[nodiscard]] inline auto decode(const uint64_t hash, const uint32_t precision,
                                 const bool round) -> Point {
  auto bbox = bounding_box(hash, precision);
  return round ? bbox.round() : bbox.center();
}

// Decode hashs into a spherical equatorial points with the given bit depth.
// If round is true, the coordinates of the points will be rounded to the
// accuracy defined by the GeoHash.
[[nodiscard]] inline auto decode(
    const Eigen::Ref<const Eigen::Matrix<uint64_t, -1, 1>>& hashs,
    const uint32_t precision, const bool center)
    -> Eigen::Matrix<Point, -1, 1> {
  auto result = Eigen::Matrix<Point, -1, 1>(hashs.size());
  for (Eigen::Index ix = 0; ix < hashs.size(); ++ix) {
    result(ix) = decode(hashs(ix), precision, center);
  }
  return result;
}

// Returns all neighbors hash clockwise from north around northwest at the given
// precision.
// 7 0 1
// 6 x 2
// 5 4 3
[[nodiscard]] auto neighbors(const uint64_t hash, const uint32_t precision)
    -> Eigen::Matrix<uint64_t, 8, 1>;

// Returns the property of the grid covering the given box: geohash of the
// minimum corner point, number of boxes in longitudes and latitudes.
[[nodiscard]] auto grid_properties(const Box& box, uint32_t precision)
    -> std::tuple<uint64_t, size_t, size_t>;

// Returns all the GeoHash codes within the box.
[[nodiscard]] auto bounding_boxes(const std::optional<Box>& box, uint32_t chars)
    -> Eigen::Matrix<uint64_t, -1, 1>;

// Returns all the GeoHash codes within the polygon.
[[nodiscard]] inline auto bounding_boxes(const Polygon& polygon, uint32_t chars)
    -> Eigen::Matrix<uint64_t, -1, 1> {
  auto box = Box();
  boost::geometry::envelope<Polygon, Box>(polygon, box);
  return bounding_boxes(box, chars);
}

// Returns the start and end indexes of the different GeoHash boxes.
[[nodiscard]] auto where(
    const Eigen::Ref<const Eigen::Matrix<uint64_t, -1, -1>>& hashs)
    -> std::map<uint64_t, std::tuple<std::tuple<int64_t, int64_t>,
                                     std::tuple<int64_t, int64_t>>>;

}  // namespace geohash::int64
