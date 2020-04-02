from typing import Any, Dict, List, Optional, Tuple


class DatabaseError(Exception):
    ...


class LockError(Exception):
    ...


class OperationalError(Exception):
    ...


class ProgrammingError(Exception):
    ...


class LevelDB:
    def __init__(self, filename: str,
                 options: Optional[Options] = None) -> None:
        ...

    def __getstate__(self) -> Tuple:
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

    def extend(self, map: Dict[bytes, Any]) -> None:
        ...

    def keys(self) -> List[bytes]:
        ...

    def update(self, map: Dict[bytes, Any]) -> None:
        ...

    def values(self, keys: Optional[List[bytes]] = None) -> List[Any]:
        ...
