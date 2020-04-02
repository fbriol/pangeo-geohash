from typing import Any, Dict, List, Optional, Tuple


class DatabaseError(Exception):
    ...


class LockError(Exception):
    ...


class OperationalError(Exception):
    ...


class ProgrammingError(Exception):
    ...


class Database:
    def __init__(self,
                 name: str,
                 create_if_missing: bool = True,
                 error_if_exists: bool = False,
                 enable_compression: bool = True) -> None:
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

    def rollback(self) -> None:
        ...

    def update(self, map: Dict[bytes, Any]) -> None:
        ...

    def values(self, keys: Optional[List[bytes]] = None) -> List[Any]:
        ...
