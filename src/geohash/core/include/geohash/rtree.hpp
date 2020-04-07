#include <Eigen/Core>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <memory>

#include "geohash/geometry.hpp"
#include "geohash/projection.hpp"

namespace geohash::geometry {

struct RTreeQueryResult {
  double distance{std::numeric_limits<double>::quiet_NaN()};
  int64_t index{-1};
};

class RTree {
 public:
  using ecef_t =
      boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian>;

  using lla_t = boost::geometry::model::point<
      double, 3, boost::geometry::cs::geographic<boost::geometry::degree>>;

  // Value handled by this object
  using value_t = std::pair<ecef_t, int64_t>;

  using rtree_t =
      boost::geometry::index::rtree<value_t, boost::geometry::index::rstar<16>>;

  RTree(const std::optional<Projection> &proj)
      : rtree_(std::make_unique<rtree_t>()),
        proj_(proj.value_or(Projection())) {}

  auto clear() { rtree_->clear(); }

  auto empty() const { rtree_->empty(); }

  auto size() const { rtree_->size(); }

  auto packing(const Eigen::Ref<const Eigen::Matrix<Point, -1, 1>> &coordinates)
      -> void;

  auto query(const Eigen::Ref<const Eigen::Matrix<Point, -1, 1>> &coordinates,
             const uint32_t k, bool within, size_t num_threads) const
      -> Eigen::Matrix<RTreeQueryResult, -1, -1>;

  auto query(const Point &coordinate, uint32_t k, bool within) const
      -> Eigen::Matrix<RTreeQueryResult, -1, 1> {
    return within ? query(coordinate, k) : query_within(coordinate, k);
  }

 private:
  std::unique_ptr<rtree_t> rtree_;
  Projection proj_;

  auto query(const Point &coordinate, uint32_t k) const
      -> Eigen::Matrix<RTreeQueryResult, -1, 1>;

  auto query_within(const Point &coordinate, uint32_t k) const
      -> Eigen::Matrix<RTreeQueryResult, -1, 1>;
};

}  // namespace geohash::geometry