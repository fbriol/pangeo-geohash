import tempfile
import os
import geohash.lock


def test_lock_thread():
    lck = geohash.lock.ThreadSynchronizer()
    assert not lck.lock.locked()
    with lck:
        assert lck.lock.locked()
    assert not lck.lock.locked()


def lock_process(path: str) -> None:
    lck = geohash.lock.ProcessSynchronizer(path)
    assert not os.path.exists(path)
    assert not lck.lock.acquired
    with lck:
        assert lck.lock.acquired
        assert os.path.exists(path)
    assert not os.path.exists(path)


def test_lock_process():
    path = tempfile.NamedTemporaryFile().name
    assert not os.path.exists(path)
    lock_process(path)
    assert not os.path.exists(path)
