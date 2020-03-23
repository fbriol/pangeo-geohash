"""
Geogrophic Index
----------------
"""
from typing import (Any, Dict, Iterable, Iterator, List, MutableMapping,
                    Optional, Tuple)
import json
import pickle
import numcodecs.abc
from . import lock
from . import core
from .core import string


class GeoHash:
    """
    Geogrophic index based on GeoHash encoding.

    Args:
        store (MutableMapping): Object managing the storage of the index
        precision (int): Accuracy of the index. By default the precision is 3
            characters. The table below gives the correspondence between the
            number of characters (i.e. the ``precision`` parameter of this
            constructor), the size of the boxes of the grid at the equator and
            the total number of boxes.

            =========  ===============  ==========
            precision  lng/lat (km)     samples
            =========  ===============  ==========
            1          4950/4950        32
            2          618.75/1237.50   1024
            3          154.69/154.69    32768
            4          19.34/38.67      1048576
            5          4.83/4.83        33554432
            6          0.60/1.21        1073741824
            =========  ===============  ==========
        compressor (numcodecs.abc.Codec, optional): Compressor
        synchronizer (lock.Synchronizer, optional): Write synchronizer
    """
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
        """Encoding of data (serialization and compression) in order to store
        it in the repository."""
        keys = data.keys()
        values = map(pickle.dumps, map(data.__getitem__, keys))
        if self._compressor:
            values: Iterator[bytes] = map(self._compressor.encode, values)
        return zip(keys, values)

    def _decode(self, keys: Iterable[bytes]) -> Iterator[List[Any]]:
        """Decoding the data (decompression and deserialization) in order to
        read it from the repository."""
        values = map(self._store.__getitem__, keys)
        if self._compressor:
            values = map(self._compressor.decode, values)
        return map(pickle.loads, values)

    def set_properties(self) -> None:
        """Definition of index properties"""
        if '.properties' in self._store:
            raise RuntimeError("index already initialized")
        self._store['.properties'] = json.dumps({
            'precision':
            self._precision,
            'compressor':
            self._compressor.get_config() if self._compressor else None
        })

    @staticmethod
    def get_properties(store) -> Dict[str, Any]:
        """Reading index properties"""
        properties = json.loads(store['.properties'])
        properties['compressor'] = numcodecs.get_codec(  # type: ignore
            properties['compressor'])
        return properties

    def update(self, data: Dict[bytes, object]) -> None:
        """Update the index with the key/value pairs from data, overwriting
        existing keys."""
        encoded_data = self._encode(
            dict((k, [v] if not isinstance(v, list) else v)
                 for k, v in data.items()))
        if self._synchronizer is not None:
            with self._synchronizer:
                self._store.update(encoded_data)
            return
        self._store.update(encoded_data)

    def append(self, data: Dict[bytes, Any]) -> None:
        """Update the index with the key/value pairs from data, appending
        existing keys with the new data."""
        keys = data.keys()
        existing_values = self._decode(keys)
        self.update(
            dict((k, v + [data[k]]) for k, v in zip(keys, existing_values)))

    def box(self, box: core.Box) -> Iterator[List[Any]]:
        """Selection of all data within the defined geographical area"""
        return self._decode(
            item
            for item in string.bounding_boxes(box, precision=self._precision))

    def __len__(self):
        return len(self._store) - 1

    def __repr__(self) -> str:
        return f"<{self.__class__.__name__} precision={self._precision}>"


def init_geohash(store: MutableMapping,
                 precision: int = 3,
                 compressor: Optional[numcodecs.abc.Codec] = None,
                 synchronizer: Optional[lock.Synchronizer] = None) -> GeoHash:
    """Creation of a GeoHash index
    """
    result = GeoHash(store, precision, compressor, synchronizer)
    result.set_properties()
    result.update(
        dict(
            (code, []) for code in string.bounding_boxes(precision=precision)))
    return result


def open_geohash(store: MutableMapping,
                 synchronizer: Optional[lock.Synchronizer] = None) -> GeoHash:
    """Open of a GeoHash index"""
    result = GeoHash(store,
                     synchronizer=synchronizer,
                     **GeoHash.get_properties(store))
    return result
