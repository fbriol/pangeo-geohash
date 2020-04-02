import pickle
import struct
import zlib
from geohash.core import storage


def test_pickle():
    p = storage.Pickle()
    s = "#" * 256
    d1 = p.dumps(s)
    d2 = pickle.dumps(s, protocol=-1)
    assert p.loads(d1) == s
    assert d1 == d2


def test_long():
    p = storage.Pickle()
    for _ in range(4096):
        s = "#" * 256
        d1 = p.dumps(s)
        d2 = pickle.dumps(s, protocol=-1)
        assert p.loads(d1) == s
        assert d1 == d2
