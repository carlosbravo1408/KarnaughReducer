import heapq
from typing import Any


class PriorityQueue:
    """
    This class is a friendly-use `heapq` wrapper
    """
    def __init__(self):
        self._elements = list()

    def empty(self) -> bool:
        return not self._elements

    def push(self, item) -> None:
        heapq.heappush(self._elements, item)

    def pop(self) -> Any:
        return heapq.heappop(self._elements)
