from typing import Any, Union, Tuple
import heapq

from minterm import Minterm


class Node:

    def __init__(
            self,
            item: Any,
            prev_node: Union['Node', None] = None,
            cost: int = 0
    ):
        self.prev_node = prev_node
        self.cost = cost
        self.item = item

    def get_branch(self) -> Tuple['Node']:
        node = self
        solution = set()
        solution.add(node)
        while node.prev_node is not None:
            node = node.prev_node
            solution.add(node)
        return tuple(solution)

    def __str__(self):
        return self.item

    def __repr__(self):
        return str(self.item)

    def __hash__(self):
        return hash(self.item)

    def __lt__(self, other: 'Node'):
        return self.cost < other.cost

    def __gt__(self, other: 'Node'):
        return self.cost > other.cost

    def __le__(self, other: 'Node'):
        return self.cost <= other.cost

    def __ge__(self, other: 'Node'):
        return self.cost >= other.cost

    def __eq__(self, other: Any):
        if isinstance(other, str):
            return str(self.item) == other
        elif isinstance(other, Minterm):
            return str(self.item) == other.minterm
        elif isinstance(other, Node):
            return self.item == other.item
        else:
            return self.item == other
