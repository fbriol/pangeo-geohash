#include "geohash/rtree.hpp"

#include <thread>

namespace geohash::geometry {

template <typename Lambda>
void dispatch(const Lambda& worker, size_t size, size_t num_threads) {
  if (num_threads == 1) {
    worker(0, size);
    return;
  }

  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }

  // List of threads responsible for parallelizing the calculation
  std::vector<std::thread> threads(num_threads);

  // Access index to the vectors required for calculation
  size_t start = 0;
  size_t shift = size / num_threads;

  // Launch and join threads
  for (auto it = std::begin(threads); it != std::end(threads) - 1; ++it) {
    *it = std::thread(worker, start, start + shift);
    start += shift;
  }
  threads.back() = std::thread(worker, start, size);

  for (auto&& item : threads) {
    item.join();
  }
}

auto RTree::packing(
    const Eigen::Ref<const Eigen::Matrix<Point, -1, 1>>& coordinates) -> void {
  auto values = std::vector<value_t>();
  auto ecef = ecef_t();

  values.reserve(coordinates.size());

  for (auto ix = std::ptrdiff_t(0); ix < coordinates.size(); ++ix) {
    const auto& point = coordinates[ix];
    proj_.forward(lla_t(point.lng, point.lat), ecef);
    values.emplace_back(std::make_pair(ecef, ix));
  }
  *rtree_ = rtree_t(values);
}

auto RTree::query(const Point& coordinate, const uint32_t k) const
    -> Eigen::Matrix<RTreeQueryResult, -1, 1> {
  // This parameter must be check in the python wrapper
  assert(k != 0);
  auto result = Eigen::Matrix<RTreeQueryResult, -1, 1>(k);
  auto ecef = ecef_t();
  auto ix = std::ptrdiff_t(0);
  proj_.forward(lla_t(coordinate.lng, coordinate.lat), ecef);
  std::for_each(rtree_->qbegin(boost::geometry::index::nearest(ecef, k)),
                rtree_->qend(), [&ecef, &ix, &result](const auto& item) {
                  result[ix++] = {boost::geometry::distance(ecef, item.first),
                                  item.second};
                });
  while (ix < k) {
    result[ix++] = RTreeQueryResult();
  }
  return result;
}

auto RTree::query_within(const Point& coordinate, const uint32_t k) const
    -> Eigen::Matrix<RTreeQueryResult, -1, 1> {
  // This parameter must be check in the python wrapper
  assert(k != 0);
  auto points = boost::geometry::model::multi_point<ecef_t>();
  auto result = Eigen::Matrix<RTreeQueryResult, -1, 1>(k);
  auto ecef = ecef_t();
  auto ix = std::ptrdiff_t(0);

  proj_.forward(lla_t(coordinate.lng, coordinate.lat), ecef);
  points.reserve(k);

  std::for_each(rtree_->qbegin(boost::geometry::index::nearest(ecef, k)),
                rtree_->qend(),
                [&ecef, &ix, &points, &result](const auto& item) {
                  points.emplace_back(item.first);
                  result[ix++] = {boost::geometry::distance(ecef, item.first),
                                  item.second};
                });

  // Are found points located around the requested point?
  if (!boost::geometry::covered_by(
          ecef,
          boost::geometry::return_envelope<boost::geometry::model::box<ecef_t>>(
              points))) {
    for (ix = 0; ix < k; ++ix) {
      result[ix] = RTreeQueryResult();
    }
  } else {
    while (ix < k) {
      result[ix++] = RTreeQueryResult();
    }
  }
  return result;
}

auto RTree::query(
    const Eigen::Ref<const Eigen::Matrix<Point, -1, 1>>& coordinates,
    const uint32_t k, const bool within, const size_t num_threads) const
    -> Eigen::Matrix<RTreeQueryResult, -1, -1> {
  auto result = Eigen::Matrix<RTreeQueryResult, -1, -1>(coordinates.size(), k);
  auto except = std::exception_ptr(nullptr);
  dispatch(
      [&](const size_t start, const size_t end) {
        try {
          for (auto ix = start; ix < end; ++ix) {
            result.row(ix) = query(coordinates[ix], k, within);
          }
        } catch (...) {
          except = std::current_exception();
        }
      },
      coordinates.size(), num_threads);
  for (auto ix = 0; ix < coordinates.size(); ++ix) {
    result.row(ix) = query(coordinates[ix], k, within);
  }
  if (except != nullptr) {
    std::rethrow_exception(except);
  }
  return result;
}

}  // namespace geohash::geometry