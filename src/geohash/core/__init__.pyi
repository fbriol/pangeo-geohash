from . import storage
from . import int64
from . import string


class Point:
    def __init__(self, lng: float = 0, lat: float = 0) -> None:
        ...

    def __repr__(self) -> str:
        ...

    @property
    def lat(self) -> float:
        ...

    @property
    def lng(self) -> float:
        ...


class Box:
    def __init__(self, min_corner: Point, max_corner: Point) -> None:
        ...

    def __repr__(self) -> str:
        ...

    def contains(self, point: Point) -> bool:
        ...

    @property
    def max_corner(self) -> Point:
        ...

    @property
    def min_corner(self) -> Point:
        ...

