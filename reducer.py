import copy
from typing import Tuple, Union

from minterm import Minterm
from node import Node
from priority_queue import PriorityQueue


class Reducer:
    def __init__(
            self,
            inputs_number: int,
            outputs: Union[str, None] = None,
            ones: Union[set, None] = None,
            not_care: Union[set, None] = None
    ):
        self.is_sum_of_products = False
        self.ones = set()
        self.not_care = set()
        self._values = set()
        self._values_checked = set()
        self.inputs_number = inputs_number
        if outputs is not None:
            for i in range(1 << inputs_number):
                if outputs[i] == '1':
                    self.ones.add(i)
                    self._values.add(i)
                elif outputs[i] != '0':
                    self.not_care.add(i)
                    self._values.add(i)
        else:
            self.ones = ones
            self.not_care = not_care if not_care else set()
            self._values = set.union(ones, not_care)
        self._minterms = set()
        self._essential_minterms = set()
        self.solutions = None

    def get_truth_table(self, number_inputs: int):
        output = []
        for k in range(1 << number_inputs):
            output.append([bool(k & (1 << n)) for n in range(number_inputs)])
        return output

    def _get_char_array_from_int(self, value, number_inputs):
        if value >= 1 << number_inputs:
            raise ArithmeticError
        return [
            int(bool(value & (1 << n))) for n in
            range(number_inputs - 1, -1, -1)
        ]

    def _value_to_add(self, input, indexes):
        value = 0
        for k in range(len(indexes)):
            value = value + ((((1 << k) & input) > 0) << indexes[k])
        return value

    def _value_to_subtract(self, input, indexes):
        value = 0
        for k in indexes:
            value1 = (1 << k) & input
            value = value + value1
        return value

    def _is_prime_implicant(self, value, indexes_to_evaluate):
        _values_checked = set()
        for k in range(1 << len(indexes_to_evaluate)):
            value_for_adding = self._value_to_add(k, indexes_to_evaluate)
            value_to_evaluate = value_for_adding + value
            if value_to_evaluate not in self._values:
                return False
            _values_checked.add(value_to_evaluate)
        self._values_checked = set.union(self._values_checked, _values_checked)
        return True

    def _search_prime_implicants(
            self,
            current_index: int,
            prime_visited: dict = {},
            is_forward: bool = True
    ):
        result_indexes = []
        prev_min_term = self._get_char_array_from_int(
            current_index, self.inputs_number
        )
        min_term = copy.deepcopy(prev_min_term)
        for j in range(self.inputs_number):
            j1 = j if is_forward else self.inputs_number - 1 - j
            j2 = j if not is_forward else self.inputs_number - 1 - j
            evaluation_indexes = copy.deepcopy(result_indexes)
            evaluation_indexes.insert(0, j2)
            prev_min_term[j1] = '-'
            _value_to_substract = self._value_to_subtract(
                current_index, evaluation_indexes
            )
            _t = tuple(prev_min_term)
            _is_implicant_prime = prime_visited[_t] \
                if _t in prime_visited \
                else self._is_prime_implicant(
                current_index - _value_to_substract, evaluation_indexes)
            if _t not in prime_visited:
                prime_visited[_t] = _is_implicant_prime
            if _is_implicant_prime:
                result_indexes.insert(0, j2)
                min_term[j1] = '-'
            else:
                prev_min_term = copy.deepcopy(min_term)
        _value_to_substract = self._value_to_subtract(
            current_index, result_indexes
        )
        _t = ''.join([str(i) for i in min_term])
        if _t not in self._minterms:
            self._minterms.add(
                Minterm(
                    _t,
                    set.intersection(
                        self.ones,
                        {
                            self._value_to_add(i, result_indexes) +
                            current_index - _value_to_substract
                            for i in range(1 << len(result_indexes))
                        }
                    )
                )
            )

    def _get_prime_implicants(self, both_way: bool=True):
        self._minterms = set()
        self._values_checked = set()
        for index in self._values:
            if index in self._values_checked:
                continue
            prime_visited = dict()
            self._search_prime_implicants(current_index=index,
                                          prime_visited=prime_visited)
            if both_way:
                self._search_prime_implicants(current_index=index,
                                              prime_visited=prime_visited,
                                              is_forward=False)

    def _get_essential_prime_implicants(self):
        for one in self.ones:
            count = 0
            last_minterm = None
            for minterm in self._minterms:
                if one in minterm.ones:
                    count += 1
                    last_minterm = minterm
                if count > 1:
                    break
            if count == 1:
                self._essential_minterms.add(last_minterm)

    def _set_is_solution(self, input_set: set) -> bool:
        return input_set == self.ones

    def _get_ones_from_branch(self, current_branch: Tuple[Node]) -> set:
        return set.union(*[term.item.ones for term in current_branch])

    def _get_all_possible_solutions(self):
        """
        This method search all the possible combinations in order to satisfy
        the expected output, for now it works with BFS algorithm
        """
        self.solutions = []
        frontier = PriorityQueue()
        visited = set()
        # if len(self._minterms) > 20:
        #     self._get_prime_implicants(False)
        #     primes = copy.deepcopy(self._minterms)
        #     start_node = Node(primes.pop())
        #     while primes:
        #         start_node = Node(primes.pop(), start_node)
        #     self.solutions = [start_node.get_branch()]
        #     return
        if self._essential_minterms:
            essential_primes = copy.deepcopy(self._essential_minterms)
            current_prime = essential_primes.pop()
            current_one_set = current_prime.ones
            start_node = Node(
                current_prime,
                cost=len(self.ones)-len(current_one_set)
            )
            while essential_primes:
                current_prime = essential_primes.pop()
                current_one_set = set.union(
                    self._get_ones_from_branch(start_node.get_branch()),
                    current_prime.ones
                )
                start_node = Node(
                    current_prime,
                    start_node,
                    len(self.ones)-len(current_one_set))
            if self._set_is_solution(current_one_set):
                self.solutions = [start_node.get_branch()]
                return
            frontier.append(start_node)
            visited.add(start_node.get_branch())
        else:
            for minterm in self._minterms:
                node = Node(minterm, cost=len(self.ones)-len(minterm.ones))
                frontier.append(node)
                visited.add(node.get_branch())

        # This works as tie-cut and stop the search after get the length
        # of the first solution.
        short_term_solution = len(self._minterms)

        while frontier:
            current_node = frontier.pop(0)
            current_branch = current_node.get_branch()
            p_solution = self._get_ones_from_branch(current_branch)
            if len(current_branch) >= short_term_solution:
                break
            for minterm in self._minterms:
                if minterm == current_node.item:
                    continue
                union_set = set.union(p_solution, minterm.ones)
                node = Node(minterm, current_node, len(self.ones)-len(union_set))
                node_branch = node.get_branch()
                if node_branch in visited or node in current_branch:
                    continue
                visited.add(node_branch)
                frontier.append(node)
                if self._set_is_solution(union_set):
                    if len(node_branch) < short_term_solution:
                        short_term_solution = len(node_branch)
                    self.solutions.append(node.get_branch())

    def reduce(self, is_sum_of_products: bool = False):
        self.is_sum_of_products = is_sum_of_products
        if is_sum_of_products:
            self.invert_values()
        self._get_prime_implicants()
        self._get_essential_prime_implicants()
        self._get_all_possible_solutions()

    def invert_values(self):
        ones = {i for i in range(1 << self.inputs_number)}
        self.ones = set.symmetric_difference(
            set.symmetric_difference(self.ones, ones), self.not_care)
        self._values = set.union(self.ones, self.not_care)
