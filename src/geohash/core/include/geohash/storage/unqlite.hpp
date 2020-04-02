#pragma once
#include "pickle.hpp"
#include <pybind11/pybind11.h>
#include <string>
#include <unqlite.h>

namespace geohash::storage::unqlite {

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

// Key/Value store
class Database {
 public:
  // Default constructor
  Database(const std::string& name, bool create_if_missing,
           bool error_if_exists, bool enable_compression);

  // Destructor
  virtual ~Database();

  // Copy constructor
  Database(const Database&) = delete;

  // Copy assignment operator
  auto operator=(const Database&) -> Database& = delete;

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
  [[nodiscard]] auto error_log() const -> std::string;

 private:
  ::unqlite* handle_{nullptr};
  Pickle pickle_{};
  bool compress_{true};

  static auto handle_rc(int rc) -> void;
};

}  // namespace geohash::storage::unqlite