from typing import (Any, Dict, Iterable, Iterator, List, MutableMapping,
                    Optional, Tuple)
import json
import pickle
import numcodecs.abc
from . import lock
from . import core


class Index:
    def __init__(self,
                 store: MutableMapping,
                 precision: int = 3,
                 compressor: Optional[numcodecs.abc.Codec] = None,
                 synchronizer: Optional[lock.Synchronizer] = None) -> None:
        self._store = store
        self._precision = precision
        self._compressor = compressor
        self._synchronizer = synchronizer

    def _encode(self,
                data: Dict[bytes, List[Any]]) -> Iterator[Tuple[bytes, bytes]]:
        keys = data.keys()
        values = map(pickle.dumps, map(data.__getitem__, keys))
        if self._compressor:
            values: Iterator[bytes] = map(self._compressor.encode, values)
        return zip(keys, values)

    def _decode(self, keys: Iterable[bytes]) -> Iterator[List[Any]]:
        values = map(self._store.__getitem__, keys)
        if self._compressor:
            values = map(self._compressor.decode, values)
        return map(pickle.loads, values)

    def set_properties(self):
        if '.properties' in self._store:
            raise RuntimeError("index already initialized")
        self._store['.properties'] = json.dumps({
            'precision':
            self._precision,
            'compressor':
            self._compressor.get_config() if self._compressor else None
        })

    @staticmethod
    def get_properties(store):
        properties = json.loads(store['.properties'])
        properties['compressor'] = numcodecs.get_codec(  # type: ignore
            properties['compressor'])
        return properties

    def update(self, data: Dict[bytes, object]) -> None:
        encoded_data = self._encode(
            dict((k, [v] if not isinstance(v, list) else v)
                 for k, v in data.items()))
        if self._synchronizer is not None:
            with self._synchronizer:
                self._store.update(encoded_data)
            return
        self._store.update(encoded_data)

    def append(self, data: Dict[bytes, Any]) -> None:
        keys = data.keys()
        existing_values = self._decode(keys)
        self.update(
            dict((k, v + [data[k]]) for k, v in zip(keys, existing_values)))

    def box(self, box: core.Box) -> Iterator[List[Any]]:
        return self._decode(item for item in core.string.bounding_boxes(
            box, precision=self._precision))

    def __len__(self):
        return len(self._store)

    def __repr__(self) -> str:
        return f"<{self.__class__.__name__} precision={self._precision}>"


def init_index(store: MutableMapping,
               precision: int = 3,
               compressor: Optional[numcodecs.abc.Codec] = None,
               synchronizer: Optional[lock.Synchronizer] = None) -> Index:
    result = Index(store, precision, compressor, synchronizer)
    result.set_properties()
    result.update(
        dict((code, [])
             for code in core.string.bounding_boxes(precision=precision)))
    return result


def open_index(store: MutableMapping,
               synchronizer: Optional[lock.Synchronizer] = None) -> Index:
    result = Index(store,
                   synchronizer=synchronizer,
                   **Index.get_properties(store))
    return result
