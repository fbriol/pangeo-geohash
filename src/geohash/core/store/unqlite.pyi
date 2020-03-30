from typing import Any, Dict, List, Optional, Tuple


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

    def __getstate__(self) -> Tuple:
        ...

    def __setstate__(self, state: Tuple) -> None:
        ...

    def __contains__(self, key: bytes) -> bool:
        ...

    def __delitem__(self, key: bytes) -> None:
        ...

    def __getitem__(self, key: bytes) -> List[Any]:
        ...

    def __len__(self) -> int:
        ...

    def __setitem__(self, key: bytes, value: Any) -> None:
        ...

    def clear(self) -> None:
        ...

    def commit(self) -> None:
        ...

    def error_log(self) -> str:
        ...

    def extend(self, map: Dict[bytes, Any]) -> None:
        ...

    def keys(self) -> List[bytes]:
        ...

    def pop(self, key: object, default: object = None) -> object:
        ...

    def popitem(self) -> tuple:
        ...

    def rollback(self) -> None:
        ...

    def setdefault(self, key: object, default: object = None) -> object:
        ...

    def update(self, map: Dict[bytes, Any]) -> None:
        ...

    def values(self, keys: Optional[List[bytes]] = None) -> List[Any]:
        ...
