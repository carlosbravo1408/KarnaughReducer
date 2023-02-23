import heapq
from typing import Union, Any


class PriorityQueue:
    def __init__(self):
        self._elements = list()

    def empty(self):
        return not self._elements

    def put(self, item, priority: Union[int, None] = None):
        if priority is None:
            heapq.heappush(self._elements, item)
        else:
            heapq.heappush(self._elements, (priority, item))

    def append(self, item, priority: Union[int, None] = None):
        self.put(item, priority)

    def pop(self, *args) -> Any:
        return heapq.heappop(self._elements)
