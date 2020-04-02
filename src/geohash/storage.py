"""
Index storage support
---------------------
"""
from typing import (Any, Dict, Iterable, Iterator, List, Mapping,
                    MutableMapping, Optional, Tuple, Union)
import weakref
import abc
import os
from .core import storage


class MutableMapping:
    """Abstract index storage class"""
    @abc.abstractmethod
    def __contains__(self, key: bytes) -> bool:
        ...

    @abc.abstractmethod
    def __delitem__(self, key: bytes) -> None:
        ...

    @abc.abstractmethod
    def __getitem__(self, key: bytes) -> list:
        ...

    @abc.abstractmethod
    def __len__(self) -> int:
        ...

    @abc.abstractmethod
    def __setitem__(self, key: bytes, value: object) -> None:
        ...

    @abc.abstractmethod
    def clear(self) -> None:
        ...

    @abc.abstractmethod
    def extend(self, map: dict) -> None:
        ...

    @abc.abstractmethod
    def keys(self) -> list:
        ...

    @abc.abstractmethod
    def update(self, map: Dict[bytes, Any]) -> None:
        ...

    @abc.abstractmethod
    def values(self, keys: Optional[List[bytes]] = None) -> List[Any]:
        ...

    @abc.abstractmethod
    def __enter__(self) -> Any:
        ...

    @abc.abstractmethod
    def __exit__(self, type, value, tb):
        ...


# class Singleton(type):
#     """Singleton meta class"""
#     _instances = weakref.WeakValueDictionary()

#     def __call__(cls, *args, **kwargs):
#         if cls not in cls._instances:
#             # This variable declaration is required to force a
#             # strong reference on the instance.
#             instance = super(Singleton, cls).__call__(*args, **kwargs)
#             cls._instances[cls] = instance
#         return cls._instances[cls]


# class LevelDB(storage.leveldb.Database, metaclass=Singleton):
#     def __init__(self, *args, **kwargs):
#         pass


# class UnQlite(unqlite.Database, MutableMapping):
#     """Storage class using SQLite.

#     Args:
#         path (str): Location of database file.
#         option (unqlite.Options, optional): options to control the behavior of
#             the database
#     """
#     def __init__(self, path: str,
#                  options: Optional[unqlite.Options] = None) -> None:
#         # normalize path
#         if path != ':mem:':
#             path = os.path.abspath(path)
#         super().__init__(path, options)

#     def __enter__(self) -> 'UnQlite':
#         return self

#     def __exit__(self, type, value, tb):
#         self.commit()

#     def __iter__(self):
#         return self.keys()
