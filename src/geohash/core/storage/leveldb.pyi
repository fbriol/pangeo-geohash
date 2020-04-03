from typing import Any, Dict, List, Optional, Tuple


class CorruptionError(RuntimeError):
    ...


class IOError(OSError):
    ...


class InvalidArgumentError(ValueError):
    ...


class NotFoundError(KeyError):
    ...


class NotSupportedError(Exception):
    ...


class Database:
    def __init__(self,
                 name: str,
                 create_if_missing: bool = True,
                 error_if_exists: bool = False,
                 enable_compression: bool = True,
                 write_buffer_size: Optional[int] = None,
                 max_open_files: Optional[int] = None,
                 lru_cache_size: Optional[int] = None,
                 block_size: Optional[int] = None,
                 block_restart_interval: Optional[int] = None,
                 max_file_size: Optional[int] = None) -> None:
        ...

    def __contains__(self, key: bytes) -> bool:
        ...

    def __delitem__(self, key: bytes) -> None:
        ...

    def __getitem__(self, key: bytes) -> List[Any]:
        ...

    def __len__(self) -> int:
        ...

    def __setitem__(self, key: bytes, value: object) -> None:
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


class LockFile:
    def __init__(self, name: str) -> None:
        ...
