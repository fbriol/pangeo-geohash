#include "geohash/store/pickle.hpp"
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <memory>
#include <pybind11/pybind11.h>

namespace geohash::store {

// A DB is a persistent ordered map from keys to values.
class LevelDB {
 public:
  // Default constructor
  LevelDB(std::string name, bool create_if_missing, bool error_if_exists,
          const std::optional<size_t>& write_buffer_size,
          const std::optional<int>& max_open_files,
          const std::optional<size_t>& lru_cache_size,
          const std::optional<size_t>& block_size,
          const std::optional<int>& block_restart_interval,
          const std::optional<size_t>& max_file_size, bool enable_compression);

  // Destructor
  virtual ~LevelDB() = default;

  // Get state of this instance
  [[nodiscard]] auto getstate() const -> pybind11::tuple;

  // Create a new instance from the information saved in the "state" variable
  static auto setstate(const pybind11::tuple& state)
      -> std::shared_ptr<LevelDB>;

  // Set the key/value pair, overwriting existing
  auto setitem(const pybind11::bytes& key, const pybind11::object& obj,
               leveldb::WriteBatch* batch) const -> void;

  // Update the database with the key/value pairs from map, overwriting existing
  // keys
  auto update(const pybind11::dict& map) const -> void;

  // Extend or create the database with the key/value pairs from map
  auto extend(const pybind11::dict& map) const -> void;

  // Return the item of the database with key key. Return an empty list if key
  // is not in the database.
  [[nodiscard]] auto getitem(const pybind11::bytes& key) const
      -> pybind11::list;

  // Read all values from the database for the keys provided
  [[nodiscard]] auto values(const std::optional<pybind11::list>& keys) const
      -> pybind11::list;

  // Remove the key from the database. Raises a KeyError if key is not int the
  // database
  auto delitem(const pybind11::bytes& key) const -> void;

  // Return a list containing all the keys from the database
  [[nodiscard]] auto keys() const -> pybind11::list;

  // Remove all items from the database
  auto clear() const -> void;

  // Return the number of items in the database
  [[nodiscard]] auto len() const -> size_t;

  // Return true if the database has a key key, else false.
  [[nodiscard]] auto contains(const pybind11::bytes& key) const -> bool;

 private:
  std::unique_ptr<leveldb::DB> handler_;
  std::unique_ptr<leveldb::Cache> cache_;
  std::string name_;
  Pickle pickle_;
};

}  // namespace geohash::store