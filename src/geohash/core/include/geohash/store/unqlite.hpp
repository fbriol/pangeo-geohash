#pragma once
#include "pickle.hpp"
#include <pybind11/pybind11.h>
#include <string>
#include <unqlite.h>

namespace geohash::store::unqlite {

/// Exception raised for errors that are related to the database
class DatabaseError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Exception raised for programming errors
class ProgrammingError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Exception raised for errors that are related to the database’s operation and
/// not necessarily under the control of the programmer
class OperationalError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Exception raised for errors that are related to lock operations
class LockError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// Options to control the behavior of a database
class Options {
 public:
  // Default constructor
  Options() = default;

  // Set compression level for the entries stored in the database. 0 for no
  // compression, 1 for best speed, 9 for best compression.
  auto set_compression_level(int value) -> void {
    if (value < 0 || value > 9) {
      throw std::invalid_argument("level must be in [0, 9]");
    }
    compression_level_ = value;
  }
  [[nodiscard]] auto get_compression_level() const -> int {
    return compression_level_;
  }

  // If true, the database will be created if it is missing.
  auto set_create_if_missing(bool value) -> void { create_if_missing_ = value; }
  [[nodiscard]] auto get_create_if_missing() const -> bool {
    return create_if_missing_;
  }

 private:
  int compression_level_{5};
  bool create_if_missing_{true};
};

// Key/Value store
class Database {
 public:
  // Default constructor
  Database(std::string filename, const std::optional<Options>& options);

  // Destructor
  virtual ~Database();

  // Copy constructor
  Database(const Database&) = delete;

  // Copy assignment operator
  auto operator=(const Database&) -> Database& = delete;

  // Move constructor
  Database(const Database&&) = delete;

  // Move assignment operator
  auto operator=(const Database &&) -> Database& = delete;

  // Get state of this instance
  [[nodiscard]] auto getstate() const -> pybind11::tuple;

  // Create a new instance from the information saved in the "state" variable
  static auto setstate(const pybind11::tuple& state)
      -> std::shared_ptr<Database>;

  // Set the key/value pair, overwriting existing
  auto setitem(const pybind11::bytes& key, const pybind11::object& obj) const
      -> void;

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

  // Commit all changes to the database
  auto commit() const -> void;

  // Rollback a write-transaction
  auto rollback() const -> void;

  // Read error log
  [[nodiscard]] auto error_log() const -> std::string {
    const char* buffer;
    int length;

    check_rc(unqlite_config(handle_, UNQLITE_CONFIG_ERR_LOG, &buffer, &length));
    if (length > 0) {
      return buffer;
    }
    return {};
  }

 private:
  std::string filename_;
  ::unqlite* handle_{nullptr};
  Pickle pickle_{};
  int compress_{5};

  static auto check_rc(int rc) -> void;
};

}  // namespace geohash::store::unqlite