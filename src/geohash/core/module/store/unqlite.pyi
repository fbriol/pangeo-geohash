from typing import Optional


class DatabaseError(Exception):
    ...


class LockError(Exception):
    ...


class OperationalError(Exception):
    ...


class ProgrammingError(Exception):
    ...


class Options:
    def __init__(self):
        ...

    @property
    def compression_level(self) -> int:
        ...

    @compression_level.setter
    def compression_level(self, value: int) -> None:
        ...

    @property
    def create_if_missing(self) -> bool:
        ...

    @create_if_missing.setter
    def create_if_missing(self, value: bool) -> None:
        ...


class Database:
    def __init__(self, filename: str,
                 options: Optional[Options] = None) -> None:
        ...

    def __contains__(self, key: bytes) -> bool:
        ...

    def __delitem__(self, key: bytes) -> None:
        ...

    def __getitem__(self, key: bytes) -> list:
        ...

    def __len__(self) -> int:
        ...

    def __setitem__(self, key: bytes, value: object) -> None:
        ...

    def clear(self) -> None:
        ...

    def commit(self) -> None:
        ...

    def error_log(self) -> str:
        ...

    def extend(self, map: dict) -> None:
        ...

    def keys(self) -> list:
        ...

    def pop(self, key: object, default: object = None) -> object:
        ...

    def popitem(self) -> tuple:
        ...

    def rollback(self) -> None:
        ...

    def setdefault(self, key: object, default: object = None) -> object:
        ...

    def update(self, map: dict) -> None:
        ...

    def values(self, keys: Optional[list] = None) -> list:
        ...
