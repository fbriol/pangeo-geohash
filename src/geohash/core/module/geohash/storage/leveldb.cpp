#include "geohash/storage/leveldb.hpp"

namespace geohash::storage::leveldb {

// ---------------------------------------------------------------------------
template <typename T>
inline auto set_option(const std::optional<T>& src, T& dst) {
  if (src.has_value()) {
    dst = src.value();
  }
}

// ---------------------------------------------------------------------------
auto Database::set_options(const bool create_if_missing,
                           const bool error_if_exists,
                           const bool enable_compression,
                           const std::optional<size_t>& write_buffer_size,
                           const std::optional<int>& max_open_files,
                           const std::optional<size_t>& lru_cache_size,
                           const std::optional<size_t>& block_size,
                           const std::optional<int>& block_restart_interval,
                           const std::optional<size_t>& max_file_size,
                           ::leveldb::Options& options) -> void {
  options.create_if_missing = create_if_missing;
  options.error_if_exists = error_if_exists;
  set_option(write_buffer_size, options.write_buffer_size);
  set_option(max_open_files, options.max_open_files);

  if (lru_cache_size.has_value()) {
    cache_ = std::unique_ptr<::leveldb::Cache>(
        ::leveldb::NewLRUCache(*lru_cache_size));
    options.block_cache = cache_.get();
  }

  set_option(block_size, options.block_size);
  set_option(block_restart_interval, options.block_restart_interval);
  set_option(max_file_size, options.max_file_size);

  options.compression = enable_compression ? ::leveldb::kSnappyCompression
                                           : ::leveldb::kNoCompression;
}

// ---------------------------------------------------------------------------
static auto slice(const pybind11::object& obj) -> ::leveldb::Slice {
  return {PyBytes_AS_STRING(obj.ptr()),
          static_cast<size_t>(PyBytes_GET_SIZE(obj.ptr()))};
}

// ---------------------------------------------------------------------------
auto inline new_iterator(::leveldb::DB* handler)
    -> std::unique_ptr<::leveldb::Iterator> {
  return std::unique_ptr<::leveldb::Iterator>(
      handler->NewIterator(::leveldb::ReadOptions()));
}

// ---------------------------------------------------------------------------
static auto handle_status(const ::leveldb::Status& status) -> void {
  if (status.ok()) {
    return;
  }
  if (status.IsCorruption()) {
    throw CorruptionError(status.ToString());
  }
  if (status.IsInvalidArgument()) {
    throw InvalidArgumentError(status.ToString());
  }
  if (status.IsIOError()) {
    throw IOError(status.ToString());
  }
  if (status.IsNotFound()) {
    throw NotFoundError(status.ToString());
  }
  if (status.IsNotSupportedError()) {
    throw NotSupportedError(status.ToString());
  }
  throw std::runtime_error("Unhandled error status: " + status.ToString());
}

// ---------------------------------------------------------------------------
Database::Database(const std::string& name, const bool create_if_missing,
                   const bool error_if_exists, const bool enable_compression,
                   const std::optional<size_t>& write_buffer_size,
                   const std::optional<int>& max_open_files,
                   const std::optional<size_t>& lru_cache_size,
                   const std::optional<size_t>& block_size,
                   const std::optional<int>& block_restart_interval,
                   const std::optional<size_t>& max_file_size)
    : handler_(nullptr), cache_(nullptr) {
  auto options = ::leveldb::Options();
  set_options(create_if_missing, error_if_exists, enable_compression,
              write_buffer_size, max_open_files, lru_cache_size, block_size,
              block_restart_interval, max_file_size, options);
  ::leveldb::DB* db;
  handle_status(::leveldb::DB::Open(options, name, &db));
  handler_.reset(db);
}

// ---------------------------------------------------------------------------
auto Database::setitem(const pybind11::bytes& key, const pybind11::object& obj,
                       ::leveldb::WriteBatch* batch) const -> void {
  pybind11::list value;
  if (PyList_Check(obj.ptr())) {
    value = obj;
  } else {
    value.append(obj);
  }
  auto data = pickle_.dumps(value);
  {
    auto gil = pybind11::gil_scoped_release();

    if (batch != nullptr) {
      batch->Put(slice(key), slice(data));
    } else {
      handle_status(
          handler_->Put(::leveldb::WriteOptions(), slice(key), slice(data)));
    }
  }
}

// ---------------------------------------------------------------------------
auto Database::update(const pybind11::dict& map) const -> void {
  auto batch = ::leveldb::WriteBatch();
  for (auto& item : map) {
    const auto key = item.first;
    if (!PyBytes_Check(item.first.ptr())) {
      throw std::runtime_error("key must be bytes: " +
                               std::string(pybind11::repr(key)));
    }
    setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
            pybind11::reinterpret_borrow<pybind11::object>(item.second),
            &batch);
  }
  {
    auto gil = pybind11::gil_scoped_release();

    handle_status(handler_->Write(::leveldb::WriteOptions(), &batch));
  }
}

// ---------------------------------------------------------------------------
auto Database::getitem(const pybind11::bytes& key) const -> pybind11::list {
  std::string value;
  {
    auto gil = pybind11::gil_scoped_release();

    auto status = handler_->Get(::leveldb::ReadOptions(), slice(key), &value);
    if (status.IsNotFound()) {
      return pybind11::list();
    }
    handle_status(status);
  }
  return pickle_.loads(value);
}

// ---------------------------------------------------------------------------
auto Database::extend(const pybind11::dict& map) const -> void {
  auto batch = ::leveldb::WriteBatch();
  for (auto& item : map) {
    const auto key = item.first;
    if (!PyBytes_Check(item.first.ptr())) {
      throw std::runtime_error("key must be bytes: " +
                               std::string(pybind11::repr(key)));
    }
    auto existing_value =
        getitem(pybind11::reinterpret_borrow<pybind11::object>(key));
    if (pybind11::len(existing_value) == 0) {
      setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
              pybind11::reinterpret_borrow<pybind11::object>(item.second),
              &batch);
    } else {
      if (PyList_Check(item.second.ptr())) {
        auto cat = pybind11::reinterpret_steal<pybind11::object>(
            PySequence_InPlaceConcat(existing_value.ptr(), item.second.ptr()));
        setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
                pybind11::reinterpret_borrow<pybind11::object>(cat), &batch);
      } else {
        existing_value.append(item.second);
        setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
                pybind11::reinterpret_borrow<pybind11::object>(existing_value),
                &batch);
      }
    }
  }
  {
    auto gil = pybind11::gil_scoped_release();

    handle_status(handler_->Write(::leveldb::WriteOptions(), &batch));
  }
}

// ---------------------------------------------------------------------------
auto Database::values(const std::optional<pybind11::list>& keys) const
    -> pybind11::list {
  auto result = pybind11::list();
  if (keys.has_value()) {
    for (auto& key : *keys) {
      if (!PyBytes_Check(key.ptr())) {
        throw std::runtime_error("key must be bytes: " +
                                 std::string(pybind11::repr(key)));
      }
      result.append(
          getitem(pybind11::reinterpret_borrow<pybind11::bytes>(key)));
    }
  } else {
    auto it = new_iterator(handler_.get());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      auto key = it->key();
      result.append(getitem(pybind11::bytes(key.data(), key.size())));
    }
    handle_status(it->status());
  }
  return result;
}

// ---------------------------------------------------------------------------
auto Database::keys() const -> pybind11::list {
  auto result = pybind11::list();

  auto it = new_iterator(handler_.get());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    auto key = it->key();
    result.append(pybind11::bytes(key.data(), key.size()));
  }
  handle_status(it->status());
  return result;
}

// ---------------------------------------------------------------------------
auto Database::len() const -> size_t {
  auto result = size_t(0);

  auto it = new_iterator(handler_.get());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ++result;
  }
  handle_status(it->status());
  return result;
}

// ---------------------------------------------------------------------------
auto Database::clear() const -> void {
  auto batch = ::leveldb::WriteBatch();
  auto it = new_iterator(handler_.get());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    batch.Delete(it->key());
  }
  handle_status(it->status());
  handle_status(handler_->Write(::leveldb::WriteOptions(), &batch));
}

// ---------------------------------------------------------------------------
auto Database::delitem(const pybind11::bytes& key) const -> void {
  handle_status(handler_->Delete(::leveldb::WriteOptions(), slice(key)));
}

// ---------------------------------------------------------------------------
auto Database::contains(const pybind11::bytes& key) const -> bool {
  std::string value;
  auto status = handler_->Get(::leveldb::ReadOptions(), slice(key), &value);
  if (status.IsNotFound()) {
    return false;
  }
  handle_status(status);
  return true;
}

}  // namespace geohash::storage::leveldb