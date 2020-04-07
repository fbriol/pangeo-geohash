#include <boost/geometry/srs/epsg.hpp>
#include <boost/geometry/srs/projection.hpp>

namespace geohash::geometry {


class Projection : public boost::geometry::srs::projection<> {
 public:
  explicit Projection(const std::string &param)
      : boost::geometry::srs::projection<>(boost::geometry::srs::proj4(param)) {
  }

  explicit Projection(const int espg = 4326)
      : boost::geometry::srs::projection<>(boost::geometry::srs::epsg(espg)) {}
};
}  // namespace geohash::geometry