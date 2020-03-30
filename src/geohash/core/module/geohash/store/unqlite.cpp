#include "geohash/store/unqlite.hpp"

namespace geohash::store::unqlite {

struct PyBytes {
  char* ptr;
  Py_ssize_t len;

  PyBytes(PyObject* value) {
    if (PyBytes_AsStringAndSize(value, &ptr, &len)) {
      throw std::runtime_error("unable to extract bytes contents");
    }
  }
};

// ---------------------------------------------------------------------------
Database::Database(std::string filename, const std::optional<Options>& options)
    : filename_(std::move(filename)) {
  auto opts = options.has_value() ? options.value() : Options();
  auto mode = opts.get_create_if_missing() ? UNQLITE_OPEN_CREATE
                                           : UNQLITE_OPEN_READWRITE;
  compress_ = opts.get_compression_level();
  check_rc(unqlite_open(&handle_, filename.c_str(), mode));
}

// ---------------------------------------------------------------------------
Database::~Database() {
  try {
    check_rc(unqlite_close(handle_));
  } catch (std::runtime_error& ex) {
    PyErr_WarnEx(PyExc_RuntimeWarning, ex.what(), 1);
  }
}

// ---------------------------------------------------------------------------
auto Database::check_rc(const int rc) const -> void {
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
auto Database::getstate() const -> pybind11::tuple {
  if (filename_ == ":mem:") {
    throw std::runtime_error("Cannot pickle in-memory databases");
  }
  return pybind11::make_tuple(filename_, compress_);
}

// ---------------------------------------------------------------------------
auto Database::setstate(const pybind11::tuple& state)
    -> std::shared_ptr<Database> {
  if (pybind11::len(state) != 2) {
    throw std::invalid_argument("invalid state");
  }
  auto options = Options();
  options.set_compression_level(state[1].cast<int>());
  return std::make_shared<Database>(state[0].cast<std::string>(), options);
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
  auto data = pickle_.dumps(value, compress_);
  auto cxx_key = PyBytes(key.ptr());
  auto cxx_data = PyBytes(data.ptr());
  {
    auto gil = pybind11::gil_scoped_release();
    check_rc(unqlite_kv_store(handle_, cxx_key.ptr,
                              static_cast<int>(cxx_key.len), cxx_data.ptr,
                              static_cast<unqlite_int64>(cxx_data.len)));
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
  const auto cxx_key = PyBytes_AsString(key.ptr());
  unqlite_int64 size;

  auto rc = unqlite_kv_fetch(handle_, cxx_key, -1, NULL, &size);
  if (rc == UNQLITE_NOTFOUND) {
    return pybind11::list();
  } else if (rc != UNQLITE_OK) {
    check_rc(rc);
  }
  auto data = pybind11::reinterpret_steal<pybind11::bytes>(
      PyBytes_FromStringAndSize(nullptr, size));
  if (data.ptr() == nullptr) {
    throw std::runtime_error("out of memory");
  }
  auto buffer = PyBytes_AsString(data.ptr());
  {
    auto gil = pybind11::gil_scoped_release();
    check_rc(unqlite_kv_fetch(handle_, cxx_key, -1, buffer, &size));
  }
  return pickle_.loads(data);
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
  check_rc(unqlite_kv_cursor_init(handle_, &cursor));

  try {
    for (unqlite_kv_cursor_first_entry(cursor);
         unqlite_kv_cursor_valid_entry(cursor);
         unqlite_kv_cursor_next_entry(cursor)) {
      check_rc(unqlite_kv_cursor_key(cursor, nullptr, &key_len));

      auto item = pybind11::reinterpret_steal<pybind11::bytes>(
          PyBytes_FromStringAndSize(nullptr, key_len));
      check_rc(unqlite_kv_cursor_key(cursor, PyBytes_AsString(item.ptr()),
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
  check_rc(unqlite_kv_cursor_init(handle_, &cursor));

  try {
    for (unqlite_kv_cursor_first_entry(cursor);
         unqlite_kv_cursor_valid_entry(cursor);
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

  check_rc(unqlite_kv_cursor_init(handle_, &cursor));
  try {
    for (unqlite_kv_cursor_first_entry(cursor);
         unqlite_kv_cursor_valid_entry(cursor);
         unqlite_kv_cursor_next_entry(cursor)) {
      check_rc(unqlite_kv_cursor_key(cursor, nullptr, &key_len));
      auto key = std::string(0, key_len + 1);
      check_rc(unqlite_kv_cursor_key(cursor, key.data(), &key_len));
      keys.emplace_back(key);
    }
    unqlite_kv_cursor_release(handle_, cursor);
  } catch (...) {
    unqlite_kv_cursor_release(handle_, cursor);
    throw;
  }

  check_rc(unqlite_begin(handle_));
  for (auto& key : keys) {
    check_rc(unqlite_kv_delete(handle_, key.data(), -1));
  }
  check_rc(unqlite_commit(handle_));
}

// ---------------------------------------------------------------------------
auto Database::delitem(const pybind11::bytes& key) const -> void {
  int rc = unqlite_kv_delete(handle_, PyBytes_AsString(key.ptr()), -1);
  if (rc == UNQLITE_NOTFOUND) {
    throw std::out_of_range(PyBytes_AsString(key.ptr()));
  } else if (rc != UNQLITE_OK) {
    check_rc(rc);
  }
}

// ---------------------------------------------------------------------------
auto Database::commit() const -> void { check_rc(unqlite_commit(handle_)); }

// ---------------------------------------------------------------------------
auto Database::rollback() const -> void { check_rc(unqlite_rollback(handle_)); }

// ---------------------------------------------------------------------------
auto Database::contains(const pybind11::bytes& key) const -> bool {
  auto size = unqlite_int64(0);
  auto rc = unqlite_kv_fetch(handle_, PyBytes_AsString(key.ptr()), -1, nullptr,
                             &size);
  if (rc == UNQLITE_NOTFOUND) {
    return false;
  } else if (rc == UNQLITE_OK) {
    return true;
  }
  check_rc(rc);
  return false;  // suppress warn from the compiler
}

}  // namespace geohash::store::unqlite