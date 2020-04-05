.. currentmodule:: geohash

API Documentation
#################

Encoding/Decoding
=================

Integer 64-bit
--------------

.. autosummary::
  :toctree: generated/

    int64.bounding_box
    int64.bounding_boxes
    int64.decode
    int64.encode
    int64.error
    int64.grid_properties
    int64.neighbors

String
------

.. autosummary::
  :toctree: generated/

    string.bounding_box
    string.bounding_boxes
    string.decode
    string.encode
    string.error
    string.grid_properties
    string.neighbors

Resource synchronization
========================

.. autosummary::
  :toctree: generated/

    lock.Lock
    lock.LockError
    lock.ProcessSynchronizer
    lock.PuppetSynchronizer
    lock.Synchronizer
    lock.ThreadSynchronizer

Index storage
=============

.. autosummary::
  :toctree: generated/

    storage.MutableMapping
    storage.UnQlite

Index
=====

.. autosummary::
  :toctree: generated/

    index.GeoHash
    index.init_geohash
    index.open_geohash
