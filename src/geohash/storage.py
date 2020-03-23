"""
Index storage support
---------------------
"""
from typing import Iterable, Iterator, Mapping, Tuple, Union
import collections
import abc
import os
import sqlite3


class SQLiteStore(collections.abc.MutableMapping):
    """Storage class using SQLite.

    Args:
        path (str): Location of database file.
        **kwargs: Keyword arguments passed through to the `sqlite3.connect`
            function.
    """
    def __init__(self, path: str, **kwargs) -> None:
        # normalize path
        if path != ':memory:':
            path = os.path.abspath(path)

        # store properties
        self.path = path
        self.kwargs = kwargs

        # open database
        self.db = sqlite3.connect(self.path,
                                  detect_types=0,
                                  isolation_level=None,
                                  check_same_thread=False,
                                  **self.kwargs)

        # get a cursor to read/write to the database
        self.cursor = self.db.cursor()

        # initialize database with our table if missing
        self.cursor.executescript('''
            BEGIN TRANSACTION;
                CREATE TABLE IF NOT EXISTS geohash
                    (k TEXT PRIMARY KEY, v BLOB);
            COMMIT TRANSACTION;
            ''')

    def __getstate__(self) -> Tuple[str, Mapping]:
        if self.path == ':memory:':
            raise RuntimeError('Cannot pickle in-memory SQLite databases')
        return self.path, self.kwargs

    def __setstate__(self, state: Iterable) -> None:
        path, kwargs = state
        self.__init__(path=path, **kwargs)

    def __enter__(self) -> 'SQLiteStore':
        return self

    def __exit__(self):
        self.close()

    def close(self) -> None:
        """Closes the underlying database."""

        # close cursor and db objects
        self.cursor.close()
        self.db.close()

    def update(
            self,
            data: Union[Mapping[bytes, bytes], Iterator[Tuple[bytes, bytes]]]
    ) -> None:
        kv = data.items() if isinstance(data, dict) else data
        self.cursor.executemany('REPLACE INTO geohash VALUES (?, ?)', kv)

    def __getitem__(self, key: bytes) -> bytes:
        value = self.cursor.execute('SELECT v FROM geohash WHERE (k = ?)',
                                    (key, ))
        for v, in value:
            return v
        raise KeyError(key)

    def __setitem__(self, key: bytes, value: bytes) -> None:
        self.update({key: value})

    def __delitem__(self, key: bytes):
        self.cursor.execute('DELETE FROM geohash WHERE (k = ?)', (key, ))
        if self.cursor.rowcount < 1:
            raise KeyError(key)

    def __contains__(self, key: bytes) -> bool:
        cs = self.cursor.execute('SELECT COUNT(*) FROM geohash WHERE (k = ?)',
                                 (key, ))
        for has, in cs:
            has = bool(has)
            return has
        return False

    def items(self) -> Iterator[Tuple[bytes, bytes]]:
        kv_list = self.cursor.execute('SELECT k, v FROM geohash')
        for k, v in kv_list:
            yield k, v

    def keys(self):
        k_list = self.cursor.execute('SELECT k FROM geohash')
        for k, in k_list:
            yield k

    def values(self):
        v_list = self.cursor.execute('SELECT v FROM geohash')
        for v, in v_list:
            yield v

    def __iter__(self):
        return self.keys()

    def __len__(self):
        count_list = self.cursor.execute('SELECT COUNT(*) FROM geohash')
        for count, in count_list:
            return count

    def clear(self):
        self.cursor.executescript('''
                BEGIN TRANSACTION;
                    DROP TABLE geohash;
                    CREATE TABLE geohash(k TEXT PRIMARY KEY, v BLOB);
                COMMIT TRANSACTION;
                ''')
