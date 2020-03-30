# import tempfile
# import os
# import geohash.storage


# def test_store_sqlite():
#     store = geohash.storage.SQLiteStore(":memory:")
#     assert len(store) == 0

#     store['A'] = 4
#     store['B'] = 5

#     len(store) == 2

#     assert store['A'] == 4
#     assert store['B'] == 5

#     assert 'A' in store

#     del store['A']

#     assert 'A' not in store

#     store['C'] = 6

#     assert set(('B', 'C')) == set(store.keys())
#     assert set((5, 6)) == set(store.values())
#     set(store.items()) == set((('B', 5), ('C', 6)))
#     set(store) == set(('B', 'C'))

#     store.clear()

#     len(store) == 0
