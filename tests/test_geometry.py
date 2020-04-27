import geohash
import numpy as np


def test_point():
    point = geohash.Point(-1, 1)
    assert point.lng == -1
    assert point.lat == 1
    assert repr(point) == "geohash.Point(-1, 1)"
    assert point.wkt() == "POINT(-1 1)"
    point = geohash.Point.read_wkt("POINT(-2 2)")
    assert point.lng == -2
    assert point.lat == 2


def test_box():
    box = geohash.Box.whole_earth()
    assert box.min_corner.lng == -180
    assert box.min_corner.lat == -90
    assert box.max_corner.lng == 180
    assert box.max_corner.lat == 90
    assert repr(box) == "geohash.Box((-180, -90), (180, 90))"
    assert box.wkt() == "POLYGON((-180 -90,-180 90,180 90,180 -90,-180 -90))"
    box = geohash.Box.read_wkt(
        "POLYGON((-180 -90,-180 90,180 90,180 -90,-180 -90))")
    assert isinstance(box, geohash.Box)


def test_polygon():
    outer = np.array([(0.0, 0.0), (0.0, 5.0), (5.0, 5.0), (5.0, 0.0),
                      (0.0, 0.0)],
                     dtype=geohash.POINT_DTYPE)
    inner = np.array([(1.0, 1.0), (4.0, 1.0), (4.0, 4.0), (1.0, 4.0),
                      (1.0, 1.0)],
                     dtype=geohash.POINT_DTYPE)
    polygon = geohash.Polygon(outer, [inner])
    assert isinstance(polygon, geohash.Polygon)
    assert str(polygon.envelope()) == "geohash.Box((0, 0), (5, 5))"
    assert polygon.wkt(
    ) == "POLYGON((0 0,0 5,5 5,5 0,0 0),(1 1,4 1,4 4,1 4,1 1))"
    polygon = geohash.Polygon.read_wkt(
        "POLYGON((0 0,0 5,5 5,5 0,0 0),(1 1,4 1,4 4,1 4,1 1))")
    assert isinstance(polygon, geohash.Polygon)
