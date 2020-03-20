from typing import Any, Union
import abc
import pathlib
import threading
import warnings
import weakref
#
import fasteners


class Synchronizer(abc.ABC):
    """Interface of Synchronizer"""
    @abc.abstractclassmethod
    def __enter__(self) -> bool:
        ...

    @abc.abstractclassmethod
    def __exit__(self, t, v, tb) -> None:
        ...


class ThreadSynchronizer(Synchronizer):
    """Provides synchronization using thread locks."""
    def __init__(self):
        self.lock = threading.Lock()

    def __enter__(self) -> bool:
        return self.lock.acquire()

    def __exit__(self, t, v, tb) -> None:
        self.lock.release()


class ProcessSynchronizer(Synchronizer):
    """Provides synchronization using file locks"""
    def __init__(self, path: Union[pathlib.Path, str]):
        if isinstance(path, str):
            path = pathlib.Path(path)
        self.path = path
        self.lock = fasteners.InterProcessLock(str(path))
        self._finalizer = weakref.finalize(
            self,
            self._cleanup,
            self.path,
            warn_message=f"Implicitly cleaning up {self!r}")

    def __repr__(self) -> str:
        return (f"<{self.__class__.__name__} {self.path!r}")

    @staticmethod
    def _cleanup(path: pathlib.Path, warn_message: str) -> None:
        if path.exists():
            path.unlink()
        warnings.warn(warn_message, ResourceWarning)

    def cleanup(self) -> None:
        """Clean up the lock file created"""
        if self._finalizer.detach():
            if self.path.exists():
                self.path.unlink()

    def __enter__(self) -> bool:
        return self.lock.acquire()

    def __exit__(self, t, v, tb) -> None:
        self.lock.release()
        self.cleanup()
