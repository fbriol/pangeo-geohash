#include "geohash/string.hpp"

#include <array>

#include "geohash/base32.hpp"
#include "geohash/int64.hpp"

namespace geohash::string {

// Handle encoding/decoding in base32
static const auto base32 = Base32();

// ---------------------------------------------------------------------------
auto Array::get_info(const pybind11::array& hashs, const ssize_t ndim)
    -> pybind11::buffer_info {
  auto info = hashs.request();
  auto dtype = hashs.dtype();
  switch (ndim) {
    case 1:
      if (info.ndim != 1) {
        throw std::invalid_argument("hashs must be a one-dimensional array");
      }
      if (dtype.kind() != 'S') {
        throw std::invalid_argument("hash must be a string array");
      }
      if (info.strides[0] > 12) {
        throw std::invalid_argument("hash length must be within [1, 12]");
      }
      break;
    default:
      if (info.ndim != 2) {
        throw std::invalid_argument("hashs must be a two-dimensional array");
      }
      if (info.strides[0] != hashs.shape(1) * info.strides[1] ||
          dtype.kind() != 'S') {
        throw std::invalid_argument("hash must be a string array");
      }
      if (info.strides[1] > 12) {
        throw std::invalid_argument("hash length must be within [1, 12]");
      }
      break;
  }
  return info;
}

// ---------------------------------------------------------------------------
auto encode(const Point& point, char* const buffer, const uint32_t precision)
    -> void {
  base32.encode(int64::encode(point, 5 * precision), buffer, precision);
}

// ---------------------------------------------------------------------------
auto encode(const Eigen::Ref<const Eigen::Matrix<Point, -1, 1>>& points,
            const uint32_t precision) -> pybind11::array {
  auto array = Array(points.size(), precision);
  auto buffer = array.buffer();

  for (Eigen::Index ix = 0; ix < points.size(); ++ix) {
    encode(points(ix), buffer, precision);
    buffer += precision;
  }
  return array.pyarray();
}

// ---------------------------------------------------------------------------
inline auto decode_bounding_box(const char* const hash, const size_t count,
                                uint32_t* precision = nullptr) -> Box {
  uint64_t integer_encoded;
  uint32_t chars;
  std::tie(integer_encoded, chars) = base32.decode(hash, count);
  if (precision != nullptr) {
    *precision = chars;
  }
  return int64::bounding_box(integer_encoded, 5 * chars);
}

// ---------------------------------------------------------------------------
auto bounding_box(const char* const hash, const size_t count) -> Box {
  return decode_bounding_box(hash, count);
}

// ---------------------------------------------------------------------------
auto decode(const char* const hash, const size_t count, const bool round)
    -> Point {
  auto bbox = bounding_box(hash, count);
  return round ? bbox.round() : bbox.center();
}

// ---------------------------------------------------------------------------
auto decode(const pybind11::array& hashs, const bool round)
    -> Eigen::Matrix<Point, -1, 1> {
  auto info = Array::get_info(hashs, 1);
  auto count = info.strides[0];
  auto result = Eigen::Matrix<Point, -1, 1>(info.shape[0]);
  auto ptr = static_cast<char*>(info.ptr);
  for (auto ix = 0LL; ix < info.shape[0]; ++ix) {
    result(ix) = decode(ptr, count, round);
    ptr += count;
  }
  return result;
}

// ---------------------------------------------------------------------------
auto neighbors(const char* const hash, const size_t count) -> pybind11::array {
  uint64_t integer_encoded;
  uint32_t precision;
  std::tie(integer_encoded, precision) = base32.decode(hash, count);

  const auto integers = int64::neighbors(integer_encoded, precision * 5);
  auto array = Array(integers.size(), precision);
  auto buffer = array.buffer();

  for (auto ix = 0; ix < integers.size(); ++ix) {
    base32.encode(integers(ix), buffer, precision);
    buffer += precision;
  }
  return array.pyarray();
}

// ---------------------------------------------------------------------------
auto bounding_boxes(const std::optional<Box>& box, const uint32_t precision)
    -> pybind11::array {
  size_t lat_step;
  size_t lng_step;
  size_t size = 0;
  uint64_t hash_sw;

  // Number of bits
  auto bits = precision * 5;

  // Calculation of the number of elements constituting the grid
  const auto boxes = box.value_or(Box({-180, -90}, {180, 90})).split();
  for (const auto& item : boxes) {
    std::tie(hash_sw, lng_step, lat_step) = int64::grid_properties(item, bits);
    size += lat_step * lng_step;
  }

  // Grid resolution in degrees
  const auto lng_lat_err = int64::error_with_precision(bits);

  // Allocation of the vector storing the different codes of the matrix created
  auto result = Array(size, precision);
  auto buffer = result.buffer();

  for (const auto& item : boxes) {
    std::tie(hash_sw, lng_step, lat_step) = int64::grid_properties(item, bits);
    const auto point_sw = int64::decode(hash_sw, bits, true);

    for (size_t lat = 0; lat < lat_step; ++lat) {
      const auto lat_shift = lat * std::get<1>(lng_lat_err);

      for (size_t lng = 0; lng < lng_step; ++lng) {
        const auto lng_shift = lng * std::get<0>(lng_lat_err);

        base32.encode(
            int64::encode({point_sw.lng + lng_shift, point_sw.lat + lat_shift},
                          bits),
            buffer, precision);
        buffer += precision;
      }
    }
  }
  return result.pyarray();
}

auto where(const pybind11::array& hashs)
    -> std::map<std::string, std::tuple<std::tuple<int64_t, int64_t>,
                                        std::tuple<int64_t, int64_t>>> {
  // Index shifts of neighboring pixels
  static const auto shift_row =
      std::array<int64_t, 8>{-1, -1, -1, 0, 1, 0, 1, 1};
  static const auto shift_col =
      std::array<int64_t, 8>{-1, 1, 0, -1, -1, 1, 0, 1};

  auto result =
      std::map<std::string, std::tuple<std::tuple<int64_t, int64_t>,
                                       std::tuple<int64_t, int64_t>>>();

  auto info = Array::get_info(hashs, 2);
  auto rows = info.shape[0];
  auto cols = info.shape[1];
  auto chars = info.strides[1];
  auto ptr = static_cast<char*>(info.ptr);
  std::string current_code;
  std::string neighboring_code;

  assert(chars <= 12);

  for (int64_t ix = 0; ix < rows; ++ix) {
    for (int64_t jx = 0; jx < cols; ++jx) {
      current_code = std::string(ptr + (ix * cols + jx) * chars, chars);

      auto it = result.find(current_code);
      if (it == result.end()) {
        result.emplace(std::make_pair(
            current_code,
            std::make_tuple(std::make_tuple(ix, ix), std::make_tuple(jx, jx))));
        continue;
      }

      for (int64_t kx = 0; kx < 8; ++kx) {
        const auto i = ix + shift_row[kx];
        const auto j = jx + shift_col[kx];

        if (i >= 0 && i < rows && j >= 0 && j < cols) {
          neighboring_code = std::string(ptr + (i * cols + j) * chars, chars);
          if (current_code == neighboring_code) {
            auto& first = std::get<0>(it->second);
            std::get<0>(first) = std::min(std::get<0>(first), i);
            std::get<1>(first) = std::max(std::get<1>(first), i);

            auto& second = std::get<1>(it->second);
            std::get<0>(second) = std::min(std::get<0>(second), j);
            std::get<1>(second) = std::max(std::get<1>(second), j);
          }
        }
      }
    }
  }
  return result;
}

}  // namespace geohash::string
