import tempfile
import shutil
import pytest
from geohash.core.storage import unqlite
from geohash.core import string


def test_interface():
    target = tempfile.NamedTemporaryFile().name
    try:
        handler = unqlite.Database(target)

        # __len__
        assert len(handler) == 0

        # __getitem__ (default value)
        assert handler[b'0'] == []

        # __settitem__
        for item in range(256):
            handler[str(item).encode()] = item

        # __len__
        assert len(handler) == 256

        # __getitem__
        for item in range(256):
            assert handler[str(item).encode()] == [item]

        # clear database
        handler.clear()
        assert len(handler) == 0

        # creation of a test set: {b'0': 0, b'1': 1, ...}
        data = dict()
        for item in range(256):
            data[str(item).encode()] = item

        # update
        handler.update(data)
        assert len(handler) == 256

        # populate DB with test set
        for item in range(256):
            assert handler[str(item).encode()] == [item]

        # extend
        handler.extend(data)
        for item in range(256):
            assert handler[str(item).encode()] == [item, item]

        # keys
        keys = handler.keys()
        assert set(keys) == set([str(item).encode() for item in range(256)])
        assert handler.values(keys) == [[int(item), int(item)]
                                        for item in keys]

        # __delitem__
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

        del handler
    finally:
        shutil.rmtree(target, ignore_errors=True)


def test_big_data():
    """Simulation of a GeoHash grid database. The database contains for each
    box a list of 10 dummy filenames.
    """
    data = dict(
        (key, ["#" * 256] * 9) for key in string.bounding_boxes(precision=3))
    subsample = list(data.keys())[256:512]
    path = tempfile.NamedTemporaryFile().name
    try:
        handler = unqlite.Database(path)

        # Populate DB
        handler.update(data)

        # Extend all boxes
        data = dict(
            (key, "#" * 256) for key in string.bounding_boxes(precision=3))
        handler.extend(data)

        # Check all values
        for item in handler.values(subsample):
            assert len(item) == 10
            for path in item:
                assert len(path) == 256
                assert path == "#" * 256

        other_instance = unqlite.Database(path)
        with pytest.raises(unqlite.OperationalError):
            # Only one instance at a time can write the DB.
            other_instance[b'000'] = None
        del other_instance

        # The current instance can still read the data.
        len(handler[b'000']) == 11

        del handler
    finally:
        shutil.rmtree(path, ignore_errors=True)
