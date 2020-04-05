import tempfile
import pickle
import shutil
import pytest
from geohash.core.storage import unqlite
from geohash.core import string


def test_interface():
    target = tempfile.NamedTemporaryFile().name
    try:
        handler = unqlite.Database(target, mode="w")

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
        handler = unqlite.Database(path, mode="w")

        # Populate DB
        handler.update(data)

        # Extend all boxes
        data = dict(
            (key, "#" * 256) for key in string.bounding_boxes(precision=3))
        handler.extend(data)

        # Check all values
        for items in handler.values(subsample):
            assert len(items) == 10
            for item in items:
                assert len(item) == 256
                assert item == "#" * 256

        other_instance = unqlite.Database(path, mode="a")
        with pytest.raises(unqlite.LockError):
            # Only one instance at a time can write to the database if a
            # transaction is pending.
            other_instance[b'000'] = 1
        del handler

        # Now the new instance can modify the database.
        other_instance = unqlite.Database(path, mode="a")
        other_instance[b'000'] = 1

        handler = unqlite.Database(path, mode="r")
        assert handler[b'000'] != [1]

        # No modification allowed in read only mode
        with pytest.raises(unqlite.ProgrammingError):
            handler[b'000'] = 1

        # Pickle support
        other_instance = pickle.loads(pickle.dumps(handler))
        assert handler[b'000'], [1]

    finally:
        shutil.rmtree(path, ignore_errors=True)
