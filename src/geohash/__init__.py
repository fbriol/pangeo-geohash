import numpy as np
from .core import Point, Box,  Polygon
from .core import int64, string

#: Numpy data type thar handle geohash points
POINT_DTYPE = np.dtype([("lng", "f8"), ("lat", "f8")])
