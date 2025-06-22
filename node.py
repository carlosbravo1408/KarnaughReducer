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

    @property
    def item(self) -> Minterm:
        return self._item

    @property
    def parents(self) -> FrozenSet['Node']:
        node = self
        solution = set()
        solution.add(node.item)
        while node._parent is not None:
            node = node._parent
            solution.add(node.item)
        # convert to hashable set
        return frozenset(solution)

    @property
    def ones(self) -> Set[int]:
        return set.union(*(node.ones for node in self.parents))

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
