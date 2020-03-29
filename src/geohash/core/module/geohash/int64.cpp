#include "geohash/int64.hpp"
#include <iostream>

namespace geohash::int64 {
namespace detail {

static constexpr auto exp232 = 4294967296.0;      // 2^32;
static constexpr auto inv_exp232 = 1.0 / exp232;  // 1 / 2^32;

// Returns true if the CPU supports Bit Manipulation Instruction Set 2 (BMI2)
inline auto has_bmi2() noexcept -> bool {
#ifdef _WIN32
  auto registers = std::array<int, 4>();
  __cpuidex(registers.data(), 7, 0);
  return (registers[1] & (1U << 8U)) != 0;
#else
  uint32_t ebx;
  asm("movl $7, %%eax;"
      "movl $0, %%ecx;"
      "cpuid;"
      "movl %%ebx, %0;"
      : "=r"(ebx)
      :
      : "eax", "ecx", "ebx");
  return (ebx & (1U << 8U)) != 0;
#endif
}

// Spread out the 32 bits of x into 64 bits, where the bits of x occupy even
// bit positions.
inline constexpr auto spread(const uint32_t x) -> uint64_t {
  auto result = static_cast<uint64_t>(x);
  result = (result | (result << 16U)) & 0X0000FFFF0000FFFFUL;
  result = (result | (result << 8U)) & 0X00FF00FF00FF00FFUL;
  result = (result | (result << 4U)) & 0X0F0F0F0F0F0F0F0FUL;
  result = (result | (result << 2U)) & 0X3333333333333333UL;
  result = (result | (result << 1U)) & 0X5555555555555555UL;
  return result;
}

// Squash the even bitlevels of X into a 32-bit word. Odd bitlevels of X are
// ignored, and may take any value.
inline constexpr auto squash(uint64_t x) -> uint32_t {
  x &= 0x5555555555555555UL;
  x = (x | (x >> 1U)) & 0X3333333333333333UL;
  x = (x | (x >> 2U)) & 0X0F0F0F0F0F0F0F0FUL;
  x = (x | (x >> 4U)) & 0X00FF00FF00FF00FFUL;
  x = (x | (x >> 8U)) & 0X0000FFFF0000FFFFUL;
  x = (x | (x >> 16U)) & 0X00000000FFFFFFFFUL;
  return static_cast<uint32_t>(x);
}

// Interleave the bits of x and y. In the result, x and y occupy even and odd
// bitlevels, respectively.
inline constexpr auto interleave(const uint32_t x, const uint32_t y)
    -> uint64_t {
  return spread(x) | (spread(y) << 1U);
}

// Deinterleave the bits of X into 32-bit words containing the even and odd
// bitlevels of X, respectively.
inline auto deinterleave(const uint64_t x) -> std::tuple<uint32_t, uint32_t> {
  return std::make_tuple(squash(x), squash(x >> 1U));
}

// Encode the position of x within the range -r to +r as a 32-bit integer.
inline constexpr auto encode_range(const double x, const double r) -> uint32_t {
  if (x >= r) {
    return std::numeric_limits<uint32_t>::max();
  }
  auto p = (x + r) / (2 * r);
  return static_cast<uint32_t>(p * exp232);
}

// Decode the 32-bit range encoding X back to a value in the range -r to +r.
inline constexpr auto decode_range(const uint32_t x, const double r) -> double {
  if (x == std::numeric_limits<uint32_t>::max()) {
    return r;
  }
  auto p = static_cast<double>(x) * inv_exp232;
  return 2 * r * p - r;
}

// Encode the position a 64-bit integer
inline constexpr auto encode(const double lat, const double lng) -> uint64_t {
  return interleave(encode_range(lat, 90), encode_range(lng, 180));
}

inline auto shrq(const double x) -> uint64_t {
  uint64_t result;
#ifdef _WIN32
  result = _mm_cvtsi128_si64(_mm_castpd_si128(_mm_loaddup_pd(&x))) >> 20U;
#else
  asm("movd %1, %%xmm0;"
      "movq %%xmm0, %%r8;"
      "shrq $20, %%r8;"
      "movq %%r8, %0;"
      : "=r"(result)
      : "r"(x)
      : "r8", "xmm0");
#endif
  return result;
}

inline auto pdepq(const uint64_t x, const uint64_t y) -> uint64_t {
  uint64_t result;
#ifdef _WIN32
  result = _pdep_u64(x, y);
#else
  asm("movq %1, %%r9;"
      "movq %2, %%r8;"
      "pdepq %%r9, %%r8, %%r10;"
      "movq %%r10, %0;"
      : "=r"(result)
      : "r"(y), "r"(x)
      : "r8", "r9", "r10");
#endif
  return result;
}

inline auto pextq(const uint64_t x, const uint64_t y) -> uint64_t {
  uint64_t result;
#ifdef _WIN32
  result = _pext_u64(x, y);
#else
  asm("movq %1, %%r9;"
      "movq %2, %%r8;"
      "pextq %%r9, %%r8, %%r10;"
      "movq %%r10, %0;"
      : "=r"(result)
      : "r"(y), "r"(x)
      : "r8", "r9", "r10");
#endif
  return result;
}

static auto encode_bim2(const double lat, const double lng) -> uint64_t {
  auto y = pdepq(
      lat == 90.0 ? 0X3FFFFFFFFFF : shrq(1.5 + (lat * 0.005555555555555556)),
      0x5555555555555555);
  auto x = pdepq(
      lng == 180.0 ? 0X3FFFFFFFFFF : shrq(1.5 + (lng * 0.002777777777777778)),
      0x5555555555555555);
  return (x << 1U) | y;
}

static auto deinterleave_bim2(const uint64_t x)
    -> std::tuple<uint32_t, uint32_t> {
  auto lat = pextq(x, 0x5555555555555555);
  auto lng = pextq(x, 0XAAAAAAAAAAAAAAAA);

  return std::make_tuple(static_cast<uint32_t>(lat),
                         static_cast<uint32_t>(lng));
}
}  // namespace detail

// Pointer to the GeoHash position encoding function.
using encoder_t = uint64_t (*)(double, double);

// Pointer to the bits extracting function.
using deinterleaver_t = std::tuple<uint32_t, uint32_t> (*)(uint64_t);

// Can the CPU use the BIM2 instruction set?
static const bool have_bim2 = detail::has_bmi2();

// Sets the encoding/decoding functions according to the CPU capacity
static encoder_t encoder = have_bim2 ? detail::encode_bim2 : detail::encode;
static deinterleaver_t deinterleaver =
    have_bim2 ? detail::deinterleave_bim2 : detail::deinterleave;

// ---------------------------------------------------------------------------
auto encode(const Point& point, const uint32_t precision) -> uint64_t {
  auto result = encoder(point.lat, point.lng);
  if (precision != 64) {
    result >>= (64 - precision);
  }
  return result;
}

// ---------------------------------------------------------------------------
auto bounding_box(const uint64_t hash, const uint32_t precision) -> Box {
  auto full_hash = hash << (64U - precision);
  auto lat_lng_int = deinterleaver(full_hash);
  auto lat = detail::decode_range(std::get<0>(lat_lng_int), 90);
  auto lng = detail::decode_range(std::get<1>(lat_lng_int), 180);
  auto lng_lat_err = error_with_precision(precision);

  return {
      {lng, lat},
      {lng + std::get<0>(lng_lat_err), lat + std::get<1>(lng_lat_err)},
  };
}

// ---------------------------------------------------------------------------
auto neighbors(const uint64_t hash, const uint32_t precision)
    -> Eigen::Matrix<uint64_t, 8, 1> {
  double lat_delta;
  double lng_delta;
  auto box = bounding_box(hash, precision);
  auto center = box.center();
  std::tie(lng_delta, lat_delta) = box.delta(false);

  return (Eigen::Matrix<uint64_t, 8, 1>() <<
              // N
              encode({center.lng, center.lat + lat_delta}, precision),
          // NE,
          encode({center.lng + lng_delta, center.lat + lat_delta}, precision),
          // E,
          encode({center.lng + lng_delta, center.lat}, precision),
          // SE,
          encode({center.lng + lng_delta, center.lat - lat_delta}, precision),
          // S,
          encode({center.lng, center.lat - lat_delta}, precision),
          // SW,
          encode({center.lng - lng_delta, center.lat - lat_delta}, precision),
          // W,
          encode({center.lng - lng_delta, center.lat}, precision),
          // NW
          encode({center.lng - lng_delta, center.lat + lat_delta}, precision))
      .finished();
}

// ---------------------------------------------------------------------------
auto grid_properties(const Box& box, const uint32_t precision)
    -> std::tuple<uint64_t, size_t, size_t> {
  auto hash_sw = encode(box.min_corner(), precision);
  auto box_sw = bounding_box(hash_sw, precision);
  auto box_ne = bounding_box(encode(box.max_corner(), precision), precision);

  auto lng_lat_err = error_with_precision(precision);
  auto lng_step = static_cast<size_t>(
      std::round((box_ne.min_corner().lng - box_sw.min_corner().lng) /
                 (std::get<0>(lng_lat_err))));
  auto lat_step = static_cast<size_t>(
      std::round((box_ne.min_corner().lat - box_sw.min_corner().lat) /
                 (std::get<1>(lng_lat_err))));

  return std::make_tuple(hash_sw, lng_step + 1, lat_step + 1);
}

// ---------------------------------------------------------------------------
auto bounding_boxes(const std::optional<Box>& box, const uint32_t precision)
    -> Eigen::Matrix<uint64_t, -1, 1> {
  size_t lat_step;
  size_t lng_step;
  size_t size = 0;
  uint64_t hash_sw;

  // Calculation of the number of elements constituting the grid
  const auto boxes = box.value_or(Box({-180, -90}, {180, 90})).split();
  for (const auto& item : boxes) {
    std::tie(hash_sw, lng_step, lat_step) = grid_properties(item, precision);
    size += lat_step * lng_step;
  }

  // Grid resolution in degrees
  const auto lng_lat_err = error_with_precision(precision);

  // Allocation of the vector storing the different codes of the matrix created
  auto result = Eigen::Matrix<uint64_t, -1, 1>(size);
  auto ix = size_t(0);

  for (const auto& item : boxes) {
    std::tie(hash_sw, lng_step, lat_step) = grid_properties(item, precision);
    auto point_sw = decode(hash_sw, precision, true);

    for (size_t lat = 0; lat < lat_step; ++lat) {
      const auto lat_shift = lat * std::get<1>(lng_lat_err);

      for (size_t lng = 0; lng < lng_step; ++lng) {
        const auto lng_shift = lng * std::get<0>(lng_lat_err);

        result(ix++) = encode(
            {point_sw.lng + lng_shift, point_sw.lat + lat_shift}, precision);
      }
    }
  }
  return result;
}

}  // namespace geohash::int64