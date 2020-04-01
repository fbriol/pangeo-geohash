#include "geohash/store/leveldb.hpp"

namespace geohash::store {

auto slice(const pybind11::object& obj) -> leveldb::Slice {
  char* ptr;
  Py_ssize_t len;
  if (PyBytes_AsStringAndSize(obj.ptr(), &ptr, &len) != 0) {
    throw std::runtime_error("unable to extract bytes contents");
  }
  return {ptr, static_cast<size_t>(len)};
}

auto new_iterator(leveldb::DB* handler) -> std::unique_ptr<leveldb::Iterator> {
  return std::unique_ptr<leveldb::Iterator>(
      handler->NewIterator(leveldb::ReadOptions()));
}

LevelDB::LevelDB(std::string name, const bool create_if_missing,
                 const bool error_if_exists,
                 const std::optional<size_t>& write_buffer_size,
                 const std::optional<int>& max_open_files,
                 const std::optional<size_t>& lru_cache_size,
                 const std::optional<size_t>& block_size,
                 const std::optional<int>& block_restart_interval,
                 const std::optional<size_t>& max_file_size,
                 const bool enable_compression)
    : handler_(nullptr), cache_(nullptr), name_(std::move(name)) {
  auto options = leveldb::Options();
  options.create_if_missing = create_if_missing;
  options.error_if_exists = error_if_exists;
  if (write_buffer_size.has_value()) {
    options.write_buffer_size = *write_buffer_size;
  }
  if (max_open_files.has_value()) {
    options.max_open_files = *max_open_files;
  }
  if (lru_cache_size.has_value()) {
    cache_ =
        std::unique_ptr<leveldb::Cache>(leveldb::NewLRUCache(*lru_cache_size));
    options.block_cache = cache_.get();
  }
  if (block_size.has_value()) {
    options.block_size = *block_size;
  }
  if (block_restart_interval.has_value()) {
    options.block_restart_interval = *block_restart_interval;
  }
  if (max_file_size.has_value()) {
    options.max_file_size = *max_file_size;
  }
  options.compression = enable_compression ? leveldb::kSnappyCompression
                                           : leveldb::kNoCompression;
  leveldb::DB* db;
  auto status = leveldb::DB::Open(options, name_, &db);
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
  handler_.reset(db);
}

// ---------------------------------------------------------------------------
auto LevelDB::setitem(const pybind11::bytes& key, const pybind11::object& obj,
                      leveldb::WriteBatch* batch) const -> void {
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
      auto status =
          handler_->Put(leveldb::WriteOptions(), slice(key), slice(data));
      if (!status.ok()) {
        throw std::runtime_error(status.ToString());
      }
    }
  }
}

// ---------------------------------------------------------------------------
auto LevelDB::update(const pybind11::dict& map) const -> void {
  auto batch = leveldb::WriteBatch();
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

    auto status = handler_->Write(leveldb::WriteOptions(), &batch);
    if (!status.ok()) {
      throw std::runtime_error(status.ToString());
    }
  }
}

// ---------------------------------------------------------------------------
auto LevelDB::getitem(const pybind11::bytes& key) const -> pybind11::list {
  std::string value;
  {
    auto gil = pybind11::gil_scoped_release();

    auto status = handler_->Get(leveldb::ReadOptions(), slice(key), &value);
    if (status.IsNotFound()) {
      return pybind11::list();
    }
    if (!status.ok()) {
      throw std::runtime_error(status.ToString());
    }
  }
  return pickle_.loads(value);
}

// ---------------------------------------------------------------------------
auto LevelDB::extend(const pybind11::dict& map) const -> void {
  auto batch = leveldb::WriteBatch();
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
    auto status = handler_->Write(leveldb::WriteOptions(), &batch);
    if (!status.ok()) {
      throw std::runtime_error(status.ToString());
    }
  }
}

// ---------------------------------------------------------------------------
auto LevelDB::values(const std::optional<pybind11::list>& keys) const
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
    auto status = it->status();
    if (!status.ok()) {
      throw std::runtime_error(status.ToString());
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
auto LevelDB::keys() const -> pybind11::list {
  auto result = pybind11::list();

  auto it = new_iterator(handler_.get());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    auto key = it->key();
    result.append(pybind11::bytes(key.data(), key.size()));
  }
  auto status = it->status();
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
  return result;
}

// ---------------------------------------------------------------------------
auto LevelDB::len() const -> size_t {
  auto result = size_t(0);

  auto it = new_iterator(handler_.get());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ++result;
  }
  auto status = it->status();
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
  return result;
}

// ---------------------------------------------------------------------------
auto LevelDB::clear() const -> void {
  auto batch = leveldb::WriteBatch();
  auto it = new_iterator(handler_.get());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    batch.Delete(it->key());
  }
  auto status = it->status();
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
  status = handler_->Write(leveldb::WriteOptions(), &batch);
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
}

// ---------------------------------------------------------------------------
auto LevelDB::delitem(const pybind11::bytes& key) const -> void {
  auto status = handler_->Delete(leveldb::WriteOptions(), slice(key));
  if (status.IsNotFound()) {
    throw std::out_of_range(PyBytes_AsString(key.ptr()));
  }
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
}

// ---------------------------------------------------------------------------
auto LevelDB::contains(const pybind11::bytes& key) const -> bool {
  std::string value;
  auto status = handler_->Get(leveldb::ReadOptions(), slice(key), &value);
  if (status.IsNotFound()) {
    return false;
  }
  if (!status.ok()) {
    throw std::runtime_error(status.ToString());
  }
  return true;
}

}  // namespace geohash::store