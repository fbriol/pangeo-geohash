import pytest
from geohash import index
from geohash import storage
from geohash import string
import geohash


def test_index():
    # Create dummy data and populate the index
    data = dict((key, key) for key in string.bounding_boxes(precision=3))
    store = storage.UnQlite(":mem:", mode="w")
    idx = index.init_geohash(store)
    idx.update(data)

    # index.box()
    box = geohash.Box(geohash.Point(-40, -40), geohash.Point(40, 40))
    boxes = list(string.bounding_boxes(box, precision=3))
    assert idx.box(box) == boxes

    with pytest.raises(RuntimeError):
        idx = index.init_geohash(store)

    idx = index.open_geohash(store)
    assert idx.precision == 3
