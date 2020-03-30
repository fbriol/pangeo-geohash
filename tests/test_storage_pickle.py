import pickle
import struct
import zlib
import geohash.core.store


def test_pickle():
    p = geohash.core.store.Pickle()
    s = "#" * 256
    d1 = p.dumps(s, compress=5)
    d2 = pickle.dumps(s, protocol=-1)
    assert p.loads(d1) == s
    assert d1 != d2
    assert zlib.decompress(d1) == d2
    assert d1[:-8] == zlib.compress(d2, level=5)
    assert d1[-8:] == struct.pack("Q", len(pickle.dumps(s, -1)))


def test_long():
    p = geohash.core.store.Pickle()
    for _ in range(4096):
        s = "#" * 256
        d1 = p.dumps(s, compress=5)
        d2 = pickle.dumps(s, protocol=-1)
        assert p.loads(d1) == s
        assert d1 != d2
        assert zlib.decompress(d1) == d2
        assert d1[:-8] == zlib.compress(d2, level=5)
        assert d1[-8:] == struct.pack("Q", len(pickle.dumps(s, -1)))

