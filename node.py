from typing import Union, Set, FrozenSet

from minterm import Minterm


class Node:

    def __init__(
            self,
            item: Minterm,
            parent: Union['Node', None] = None,
            cost: int = 0
    ):
        self._parent = parent
        self._cost = cost
        self._item = item

    @property
    def cost(self) -> int:
        return self._cost

    def get_branch(self) -> Tuple['Node']:
    @property
    def item(self) -> Minterm:
        return self._item

        node = self
        solution = set()
        solution.add(node)
        while node.prev_node is not None:
            node = node.prev_node
            solution.add(node)
        return tuple(solution)

    def __str__(self):
        return str(self.item)

    def __repr__(self):
        return str(self.item)

    def __hash__(self):
        return hash(self.item)

    def __lt__(self, other: 'Node'):
        return self.cost < other.cost

    def __eq__(self, other: 'Node'):
        return self.item == other.item
