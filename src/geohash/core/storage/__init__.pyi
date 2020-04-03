from . import leveldb
from . import unqlite


class Pickle:
    def dumps(self, obj: object, compress: int = 0) -> bytes:
        ...

    def loads(self, bytes_object: bytes) -> object:
        ...
