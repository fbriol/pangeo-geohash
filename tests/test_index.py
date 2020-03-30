# import geohash
# import geohash.index
# import geohash.lock
# import geohash.storage
# import numcodecs


# def _laucher(store, synchronizer):
#     index = geohash.index.init_geohash(store, 1, numcodecs.BZ2(), synchronizer)
#     assert len(index) == 32
#     assert str(index) == '<GeoHash precision=1>'
#     index.append({b'm': 1, b'q': 2, b'8': 3})
#     assert set(item for items in index.box(
#         geohash.Box(geohash.Point(-180, -90), geohash.Point(180, 90)))
#                for item in items) == {1, 2, 3}
#     index.update({b'm': 5})
#     assert set(item for items in index.box(
#         geohash.Box(geohash.Point(-180, -90), geohash.Point(180, 90)))
#                for item in items) == {2, 3, 5}
#     index.append({b'm': 50})
#     assert set(item for items in index.box(
#         geohash.Box(geohash.Point(-180, -90), geohash.Point(180, 90)))
#                for item in items) == {2, 3, 5, 50}


# def test_index():
#     _laucher(dict(), None)
#     _laucher(geohash.storage.SQLiteStore(":memory:"),
#              geohash.lock.ThreadSynchronizer())
