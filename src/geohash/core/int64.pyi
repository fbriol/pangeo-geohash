from typing import Optional, Tuple, overload
import numpy
from . import Point, Box


def bounding_box(hash: int, precision: int = 64) -> Box:
    ...


def bounding_boxes(box: Optional[Box] = None,
                   precision: int = 5) -> numpy.ndarray:
    ...


@overload
def decode(hash: int, precision: int = 64, round: bool = False):
    ...


def decode(hashs: numpy.ndarray[numpy.uint64],
           precision: int = 64,
           round: bool = False) -> numpy.ndarray[Point]:
    ...


@overload
def encode(point: Point, precision: int = 64) -> int:
    ...


def encode(points: numpy.ndarray[Point],
           precision: int = 64) -> numpy.ndarray[numpy.uint64]:
    ...


def error(precision: int) -> Tuple[float, float]:
    ...


def grid_properties(box: Optional[Box] = None,
                    precision: int = 64) -> Tuple[int, int, int]:
    ...


def neighbors(box: int, precision: int = 64) -> numpy.ndarray[numpy.uint64]:
    ...
