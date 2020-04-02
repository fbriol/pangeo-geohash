import os
import tempfile
import shutil
from geohash.core import storage
from geohash.core import string


def test_interface():
    target = tempfile.NamedTemporaryFile().name
    try:
        handler = storage.leveldb.Database(target)
        assert len(handler) == 0
        assert handler[b'0'] == []
        for item in range(256):
            handler[str(item).encode()] = item
        assert len(handler) == 256
        for item in range(256):
            assert handler[str(item).encode()] == [item]
        handler.clear()
        assert len(handler) == 0
        data = dict()
        for item in range(256):
            data[str(item).encode()] = item
        handler.update(data)
        assert len(handler) == 256
        for item in range(256):
            assert handler[str(item).encode()] == [item]
        handler.extend(data)
        for item in range(256):
            assert handler[str(item).encode()] == [item, item]
        keys = handler.keys()
        print(keys)
        assert set(keys) == set([str(item).encode() for item in range(256)])
        assert handler.values(keys) == [[int(item), int(item)]
                                        for item in keys]
        for item in range(256):
            if item % 2 == 0:
                del handler[str(item).encode()]
        for item in range(256):
            key = str(item).encode()
            if item % 2 == 0:
                handler[key] == []
                assert key not in handler
            else:
                handler[key] == [item, item]
                assert key in handler
    finally:
        shutil.rmtree(target, ignore_errors=True)


def test_big_data():
    data = dict(
        (key, ["#" * 256] * 10) for key in string.bounding_boxes(precision=3))
    subsample = list(data.keys())[256:512]
    path = tempfile.NamedTemporaryFile().name
    try:
        handler = storage.leveldb.Database(path)
        handler.extend(data)
        data = dict(
            (key, "#" * 256) for key in string.bounding_boxes(precision=3))
        handler.extend(data)
        for item in handler.values(subsample):
            assert len(item) == 11
            assert len(item[0]) == 256
            assert item[0] == "#" * 256

        try:
            storage.leveldb.Database(path)
            assert False
        except storage.leveldb.IOError:
            pass
        assert len(handler[b'000']) == 11

    finally:
        shutil.rmtree(path, ignore_errors=True)
