#include "geohash/storage/unqlite.hpp"
#include <snappy.h>

namespace geohash::storage::unqlite {

struct Slice {
  char* ptr;
  Py_ssize_t len;

  explicit Slice(const pybind11::object& value)
      : ptr(PyBytes_AS_STRING(value.ptr())),
        len(PyBytes_GET_SIZE(value.ptr())) {}
};

// ---------------------------------------------------------------------------
auto Database::handle_rc(const int rc) -> void {
  // No errors detected, we have nothing more to do.
  if (rc == UNQLITE_OK) {
    return;
  }

  // If the log reading has failed, a static error message is returned
  switch (rc) {
    case UNQLITE_NOMEM:
      throw OperationalError("Out of memory");
    case UNQLITE_ABORT:
      throw OperationalError("Another thread have released this instance");
    case UNQLITE_IOERR:
      throw OperationalError("IO error");
    case UNQLITE_CORRUPT:
      throw OperationalError("Corrupt pointer");
    case UNQLITE_LOCKED:
      throw LockError("Forbidden operation");
    case UNQLITE_BUSY:
      throw LockError("The database file is locked");
    case UNQLITE_DONE:
      throw OperationalError("Operation done");
    case UNQLITE_PERM:
      throw OperationalError("Permission error");
    case UNQLITE_NOTIMPLEMENTED:
      throw ProgrammingError(
          "Method not implemented by the underlying Key/Value storage engine");
    case UNQLITE_NOTFOUND:
      throw ProgrammingError("No such record");
    case UNQLITE_NOOP:
      throw ProgrammingError("No such method");
    case UNQLITE_INVALID:
      throw ProgrammingError("Invalid parameter");
    case UNQLITE_EOF:
      throw OperationalError("End Of Input");
    case UNQLITE_UNKNOWN:
      throw ProgrammingError("Unknown configuration option");
    case UNQLITE_LIMIT:
      throw OperationalError("Database limit reached");
    case UNQLITE_EXISTS:
      throw OperationalError("Record exists");
    case UNQLITE_EMPTY:
      throw ProgrammingError("Empty record");
    case UNQLITE_COMPILE_ERR:
      throw ProgrammingError("Compilation error");
    case UNQLITE_VM_ERR:
      throw OperationalError("Virtual machine error");
    case UNQLITE_FULL:
      throw OperationalError("Full database");
    case UNQLITE_CANTOPEN:
      throw ProgrammingError("Unable to open the database file");
    case UNQLITE_READ_ONLY:
      throw ProgrammingError("Read only Key/Value storage engine");
    case UNQLITE_LOCKERR:
      throw OperationalError("Locking protocol error");
    default:
      break;
  }
  throw DatabaseError("Unknown error code. (" + std::to_string(rc) + ")");
}

// ---------------------------------------------------------------------------
auto Database::error_log() const -> std::string {
  const char* buffer;
  int length;

  handle_rc(unqlite_config(handle_, UNQLITE_CONFIG_ERR_LOG, &buffer, &length));
  if (length > 0) {
    return buffer;
  }
  return {};
}

// ---------------------------------------------------------------------------
Database::Database(const std::string& name, bool create_if_missing,
                   bool error_if_exists, bool enable_compression) {
  auto mode = create_if_missing ? UNQLITE_OPEN_CREATE : UNQLITE_OPEN_READWRITE;
  compress_ = enable_compression;
  handle_rc(unqlite_open(&handle_, name.c_str(), mode));
}

// ---------------------------------------------------------------------------
Database::~Database() {
  try {
    handle_rc(unqlite_close(handle_));
  } catch (std::runtime_error& ex) {
    PyErr_WarnEx(PyExc_RuntimeWarning, ex.what(), 1);
  }
}

// ---------------------------------------------------------------------------
static auto compress(const pybind11::bytes& bytes) -> pybind11::bytes {
  auto slice = Slice(bytes);
  auto len = snappy::MaxCompressedLength(slice.len);
  auto result = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, len));

  {
    auto gil = pybind11::gil_scoped_release();
    snappy::RawCompress(slice.ptr, slice.len, PyBytes_AS_STRING(result.ptr()),
                        &len);
  }
  if (_PyBytes_Resize(&result.ptr(), len) < 0) {
    throw pybind11::error_already_set();
  }
  return result;
}

// ---------------------------------------------------------------------------
static auto uncompress(const pybind11::bytes& bytes) -> pybind11::bytes {
  auto slice = Slice(bytes);
  auto uncompressed_len = size_t(0);
  if (!snappy::GetUncompressedLength(slice.ptr, slice.len, &uncompressed_len)) {
    return bytes;
  }
  auto result = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, uncompressed_len + 1));
  {
    auto gil = pybind11::gil_scoped_release();
    snappy::RawUncompress(slice.ptr, static_cast<size_t>(slice.len),
                          PyBytes_AS_STRING(result.ptr()));
  }
  return result;
}

// ---------------------------------------------------------------------------
auto Database::setitem(const pybind11::bytes& key,
                       const pybind11::object& obj) const -> void {
  pybind11::list value;
  if (PyList_Check(obj.ptr())) {
    value = obj;
  } else {
    value.append(obj);
  }
  auto bytes_object = pickle_.dumps(value);
  auto data = compress_ ? compress(bytes_object) : bytes_object;
  auto slice_key = Slice(key);
  auto slice_data = Slice(data);
  {
    auto gil = pybind11::gil_scoped_release();
    handle_rc(unqlite_kv_store(handle_, slice_key.ptr,
                               static_cast<int>(slice_key.len), slice_data.ptr,
                               static_cast<unqlite_int64>(slice_data.len)));
  }
}

// ---------------------------------------------------------------------------
auto Database::update(const pybind11::dict& map) const -> void {
  for (auto& item : map) {
    const auto key = item.first;
    if (!PyBytes_Check(item.first.ptr())) {
      throw std::runtime_error("key must be bytes: " +
                               std::string(pybind11::repr(key)));
    }
    setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
            pybind11::reinterpret_borrow<pybind11::object>(item.second));
  }
}

// ---------------------------------------------------------------------------
auto Database::getitem(const pybind11::bytes& key) const -> pybind11::list {
  const auto ptr_key = PyBytes_AS_STRING(key.ptr());
  unqlite_int64 size;

  auto rc = unqlite_kv_fetch(handle_, ptr_key, -1, nullptr, &size);
  if (rc == UNQLITE_NOTFOUND) {
    return pybind11::list();
  }
  if (rc != UNQLITE_OK) {
    handle_rc(rc);
  }
  auto data = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, size));
  if (data.ptr() == nullptr) {
    throw std::runtime_error("out of memory");
  }
  auto buffer = PyBytes_AS_STRING(data.ptr());
  {
    auto gil = pybind11::gil_scoped_release();
    handle_rc(unqlite_kv_fetch(handle_, ptr_key, -1, buffer, &size));
  }
  return pickle_.loads(uncompress(data));
}

// ---------------------------------------------------------------------------
auto Database::extend(const pybind11::dict& map) const -> void {
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
              pybind11::reinterpret_borrow<pybind11::object>(item.second));
    } else {
      if (PyList_Check(item.second.ptr())) {
        auto cat = pybind11::reinterpret_steal<pybind11::object>(
            PySequence_InPlaceConcat(existing_value.ptr(), item.second.ptr()));
        setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
                pybind11::reinterpret_borrow<pybind11::object>(cat));
      } else {
        existing_value.append(item.second);
        setitem(pybind11::reinterpret_borrow<pybind11::object>(key),
                pybind11::reinterpret_borrow<pybind11::object>(existing_value));
      }
    }
  }
}

// ---------------------------------------------------------------------------
auto Database::values(const std::optional<pybind11::list>& keys) const
    -> pybind11::list {
  auto result = pybind11::list();
  for (auto& key : keys.has_value() ? keys.value() : this->keys()) {
    if (!PyBytes_Check(key.ptr())) {
      throw std::runtime_error("key must be bytes: " +
                               std::string(pybind11::repr(key)));
    }
    result.append(getitem(pybind11::reinterpret_borrow<pybind11::bytes>(key)));
  }
  return result;
}

// ---------------------------------------------------------------------------
auto Database::keys() const -> pybind11::list {
  int key_len;
  auto result = pybind11::list();

  unqlite_kv_cursor* cursor = nullptr;
  handle_rc(unqlite_kv_cursor_init(handle_, &cursor));

  try {
    for (unqlite_kv_cursor_first_entry(cursor);
         unqlite_kv_cursor_valid_entry(cursor) != 0;
         unqlite_kv_cursor_next_entry(cursor)) {
      handle_rc(unqlite_kv_cursor_key(cursor, nullptr, &key_len));

      auto item = pybind11::reinterpret_steal<pybind11::bytes>(
          PyBytes_FromStringAndSize(nullptr, key_len));
      handle_rc(unqlite_kv_cursor_key(cursor, PyBytes_AS_STRING(item.ptr()),
                                      &key_len));
      result.append(item);
    }
    unqlite_kv_cursor_release(handle_, cursor);
  } catch (...) {
    unqlite_kv_cursor_release(handle_, cursor);
    throw;
  }
  return result;
}

// ---------------------------------------------------------------------------
auto Database::len() const -> size_t {
  auto result = size_t(0);

  unqlite_kv_cursor* cursor = nullptr;
  handle_rc(unqlite_kv_cursor_init(handle_, &cursor));

  try {
    for (unqlite_kv_cursor_first_entry(cursor);
         unqlite_kv_cursor_valid_entry(cursor) != 0;
         unqlite_kv_cursor_next_entry(cursor)) {
      ++result;
    }
    unqlite_kv_cursor_release(handle_, cursor);
  } catch (...) {
    unqlite_kv_cursor_release(handle_, cursor);
    throw;
  }
  return result;
}

// ---------------------------------------------------------------------------
auto Database::clear() const -> void {
  unqlite_kv_cursor* cursor = nullptr;
  auto key_len = int(0);
  auto keys = std::vector<std::string>();

  handle_rc(unqlite_kv_cursor_init(handle_, &cursor));
  try {
    for (unqlite_kv_cursor_first_entry(cursor);
         unqlite_kv_cursor_valid_entry(cursor) != 0;
         unqlite_kv_cursor_next_entry(cursor)) {
      handle_rc(unqlite_kv_cursor_key(cursor, nullptr, &key_len));
      auto key = std::string(key_len + 1, '\0');
      handle_rc(unqlite_kv_cursor_key(cursor, key.data(), &key_len));
      keys.emplace_back(key);
    }
    unqlite_kv_cursor_release(handle_, cursor);
  } catch (...) {
    unqlite_kv_cursor_release(handle_, cursor);
    throw;
  }

  handle_rc(unqlite_begin(handle_));
  for (auto& key : keys) {
    handle_rc(unqlite_kv_delete(handle_, key.data(), -1));
  }
  handle_rc(unqlite_commit(handle_));
}

// ---------------------------------------------------------------------------
auto Database::delitem(const pybind11::bytes& key) const -> void {
  int rc = unqlite_kv_delete(handle_, PyBytes_AS_STRING(key.ptr()), -1);
  if (rc == UNQLITE_NOTFOUND) {
    throw std::out_of_range(PyBytes_AS_STRING(key.ptr()));
  }
  if (rc != UNQLITE_OK) {
    handle_rc(rc);
  }
}

// ---------------------------------------------------------------------------
auto Database::commit() const -> void { handle_rc(unqlite_commit(handle_)); }

// ---------------------------------------------------------------------------
auto Database::rollback() const -> void {
  handle_rc(unqlite_rollback(handle_));
}

// ---------------------------------------------------------------------------
auto Database::contains(const pybind11::bytes& key) const -> bool {
  auto size = unqlite_int64(0);
  auto rc = unqlite_kv_fetch(handle_, PyBytes_AS_STRING(key.ptr()), -1, nullptr,
                             &size);
  if (rc == UNQLITE_NOTFOUND) {
    return false;
  }
  if (rc == UNQLITE_OK) {
    return true;
  }
  handle_rc(rc);
  return false;  // suppress warn from the compiler
}

}  // namespace geohash::storage::unqlite