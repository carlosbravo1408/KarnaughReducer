import copy
from typing import Union, Set

from .minterm import Minterm
from .node import Node
from .priority_queue import PriorityQueue


class Reducer:
    def __init__(
            self,
            num_input: int,
            ones: Union[set, None] = None,
            not_care: Union[set, None] = None,
            is_sop: bool = True,
    ):
        self.__original_ones = ones
        self._ones = copy.deepcopy(ones)
        self._not_care = not_care
        self._values = set.union(self._ones, self._not_care)
        self._inputs_number = num_input
        self._is_sop = is_sop
        if not is_sop:
            self._invert_input()

    def _invert_input(self):
        self._ones = {i for i in range(1<<self._inputs_number)} \
             - self._ones \
             - self._not_care
        self._values = set.union(self._ones, self._not_care)


    def minimize(self):
        prime_implicants = self._get_prime_implicants()
        essential_prime_implicants = self._get_essential_prime_implicants(
            prime_implicants
        )
        prime_implicants = prime_implicants - essential_prime_implicants
        prime_implicants = prime_implicants - self._get_non_prime_implicants(
            prime_implicants)
        return essential_prime_implicants, prime_implicants

    def _get_prime_implicants(self):
        minterms = [
            Minterm(f'{v:0{self._inputs_number}b}', {v}) for v in self._values
        ]
        value = 0
        while value < self._inputs_number and len(minterms) > 1:
            comparatives = set()
            visited = [False for _ in range(len(minterms))]
            for i in range(len(minterms) - 1):
                for j in range(i + 1, len(minterms)):
                    if minterms[i].hamming_distance(minterms[j]) == 1:
                        visited[j] = visited[i] = True
                        comparatives.add(
                            minterms[j].create_implicant(minterms[i])
                        )
            for i in range(len(visited)):
                if visited[i]:
                    continue
                comparatives.add(minterms[i])
            if comparatives == set(minterms):
                break
            value += 1
            minterms = list(comparatives)
        return set(minterms)

    def _get_non_prime_implicants(self, minterms: set):
        non_implicants = set()
        for minterm in minterms:
            if len(minterm.ones.intersection(self._ones)) == 0:
                non_implicants.add(minterm)
        return non_implicants

    def _get_essential_prime_implicants(self, minterms: set):
        essential_prime_implicants = set()
        for one in self._ones:
            cnt = 0
            last_minterm = None
            for minterm in minterms:
                if one in minterm.ones:
                    cnt += 1
                    last_minterm = minterm
                if cnt > 1:
                    break
            if cnt == 1:
                essential_prime_implicants.add(last_minterm)
        return essential_prime_implicants

    def _sop2pos(self, solution: Set):
        """
        Convert Sum of products to Product of Sums
        """
        new_solution = set()
        for subset in solution:
            new_solution.add(frozenset(minterm.sop2pos() for minterm in subset))
        return new_solution

    def get_all_solutions(self, max_solutions: int = 5):
        essential_prime_implicants, prime_implicants = self.minimize()
        shortest_solution_len = len(essential_prime_implicants) \
            + len(prime_implicants)
        solutions = set()
        frontier = PriorityQueue()
        configuration_visited = set()
        if len(essential_prime_implicants) > 0:
            current_prime = essential_prime_implicants.pop()
            start_node = Node(
                item=current_prime,
                cost=len(self._ones - current_prime.ones),
            )
            while essential_prime_implicants:
                current_prime = essential_prime_implicants.pop()
                current_ones = set.union(start_node.ones, current_prime.ones)
                cost = len(self._ones - current_ones)
                start_node = Node(
                    item=current_prime,
                    parent=start_node,
                    cost = cost,
                )
            if len(self._ones - start_node.ones) == 0:
                solutions = {start_node.parents}
                if not self._is_sop:
                    solutions = self._sop2pos(solutions)
                return solutions
            frontier.push(start_node)
            configuration_visited.add(start_node.parents)
        else:
            for minterm in prime_implicants:
                node = Node(
                    item=minterm,
                    cost=len(self._ones - minterm.ones),
                )
                frontier.push(node)
                configuration_visited.add(node.parents)

        # Dijkstra algorithm
        while not frontier.empty():
            current_node = frontier.pop()
            current_configuration = current_node.parents
            current_ones = current_node.ones
            if 0 < max_solutions <= len(solutions):
                break
            if len(current_configuration) > shortest_solution_len:
                continue
            for minterm in prime_implicants:
                if minterm == current_node.item:
                    continue
                _ones = set.union(current_ones, minterm.ones)
                node = Node(
                    item=minterm,
                    cost=len(self._ones - _ones),
                    parent=current_node,
                )
                node_configuration = node.parents
                if node_configuration in configuration_visited:
                    continue
                if node in current_configuration:
                    continue
                if node.cost >= current_node.cost:
                    continue
                if len(self._ones - node.ones) == 0:
                    if len(node_configuration) < shortest_solution_len:
                        shortest_solution_len = len(node_configuration)
                        solutions = set()
                    if len(node_configuration) <= shortest_solution_len:
                        solutions.add(node_configuration)
                else:
                    frontier.push(node)
                configuration_visited.add(node_configuration)
        if not self._is_sop:
            solutions = self._sop2pos(solutions)
        return solutions
