"""
Index storage support
---------------------
"""
from typing import Any, Dict, List, MutableMapping, Optional
import abc
import io
import os
import pathlib
import shutil
import uuid
import weakref
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

    def __iter__(self):
        return self.keys()


class SymlinksLeveldb:
    def __init__(self, path: pathlib.Path):
        self.target = path.joinpath(str(uuid.uuid4()))
        self.target.mkdir()
        wd = os.getcwd()
        try:
            os.chdir(self.target)
            for root, dirs, files in os.walk(path):
                dirs.clear()
                for item in files:
                    if item == "LOCK":
                        continue
                    os.symlink(
                        os.path.relpath(pathlib.Path(root, item), self.target),
                        item)
        finally:
            os.chdir(wd)

    def cleanup(self):
        shutil.rmtree(self.target, ignore_errors=True)


class ReadOnlyLevelDB(storage.leveldb.Database, MutableMapping):
    def __init__(self, name: str, **kwargs):
        path = pathlib.Path(name).absolute()
        if not path.exists() or not path.is_dir():
            raise ValueError(f"unable to open DB: {name}")

        lck = storage.leveldb.LockFile(str(path.joinpath("LOCK")))
        symlinks = SymlinksLeveldb(path)
        del lck

        self._symlink = weakref.finalize(symlinks, symlinks.cleanup)
        super().__init__(str(symlinks.target), **kwargs)

    def __enter__(self) -> 'ReadOnlyLevelDB':
        return self

    def __exit__(self, type, value, tb):
        return

    def __setitem__(self, key: bytes, value: object) -> None:
        raise io.UnsupportedOperation("not writable")

    def update(self, map: Dict[bytes, Any]) -> None:
        raise io.UnsupportedOperation("not writable")

    def extend(self, map: dict) -> None:
        raise io.UnsupportedOperation("not writable")

    def __delitem__(self, key: bytes) -> None:
        raise io.UnsupportedOperation("not writable")

    def clear(self) -> None:
        raise io.UnsupportedOperation("not writable")


class LevelDB(storage.leveldb.Database, MutableMapping):
    def __init__(self, name: str, **kwargs):
        super().__init__(str(pathlib.Path(name).absolute()), **kwargs)

    def __enter__(self) -> 'LevelDB':
        return self

    def __exit__(self, type, value, tb):
        return


class UnQlite(storage.unqlite.Database, MutableMapping):
    """Storage class using SQLite.

    Args:
        path (str): Location of database file.
        create_if_missing (bool): If true, the database will be created if
            it is missing.
        error_if_exists (bool): If true, an error is raised if the database
            already exists.
        enable_compression (bool): If true the data is compressed when
            writing
    """
    def __init__(self, name: str, **kwargs):
        # normalize path
        if name != ':mem:':
            name = os.path.abspath(name)
        super().__init__(name, **kwargs)

    def __enter__(self) -> 'UnQlite':
        return self

    def __exit__(self, type, value, tb):
        self.commit()
