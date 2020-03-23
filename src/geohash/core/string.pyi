from typing import Optional, Tuple, overload
import numpy
from . import Point, Box


def bounding_box(hash: str) -> Box:
    ...


def bounding_boxes(box: Optional[Box] = None,
                   precision: int = 1) -> numpy.ndarray[bytes]:
    ...


@overload
def decode(hash: str, round: bool = False) -> Point:
    ...


def decode(hashs: numpy.ndarray[bytes],
           round: bool = False) -> numpy.ndarray[Point]:
    ...


@overload
def encode(point: Point, precision: int = 12) -> bytes:
    ...


def encode(points: numpy.ndarray[Point],
           precision: int = 12) -> numpy.ndarray[bytes]:
    ...


def error(precision: int) -> Tuple[float, float]:
    ...


def grid_properties(box: Optional[Box] = None,
                    precision: int = 12) -> Tuple[int, int, int]:
    ...


def neighbors(box: str) -> numpy.ndarray[bytes]:
    ...
