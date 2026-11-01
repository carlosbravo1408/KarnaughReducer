from dataclasses import dataclass
from typing import Iterable, Iterator, List, Optional, Set, Tuple

from .cube import Cube


@dataclass(frozen=True)
class ReductionResult:
    """
    Minimal solutions of a boolean function. Every cube groups ones when
    ``is_sop`` is True and zeros otherwise; ``is_sop`` applies to the whole
    result, not to each cube.
    """
    num_inputs: int
    is_sop: bool
    solutions: List[Tuple[Cube, ...]]

    def to_pla(self) -> List[List[str]]:
        return [
            [cube.to_pla(self.num_inputs) for cube in solution]
            for solution in self.solutions
        ]

    def __str__(self) -> str:
        return f"{'SoP' if self.is_sop else 'PoS'} {self.to_pla()}"


class Reducer:
    def __init__(
            self,
            num_input: int,
            ones: Optional[Iterable[int]] = None,
            not_care: Optional[Iterable[int]] = None,
            is_sop: bool = True,
    ):
        # _ones holds the cells to cover: ones in SoP, zeros in PoS
        self._ones = set(ones or ())
        self._not_care = set(not_care or ())
        self._values = self._ones | self._not_care
        self._inputs_number = num_input
        self._is_sop = is_sop
        if not is_sop:
            self._invert_input()

    def _invert_input(self):
        self._ones = set(range(1 << self._inputs_number)) \
            - self._ones \
            - self._not_care
        self._values = self._ones | self._not_care

    def minimize(self) -> Tuple[Set[Cube], Set[Cube]]:
        prime_implicants = self._get_prime_implicants()
        essential_prime_implicants = self._get_essential_prime_implicants(
            prime_implicants
        )
        prime_implicants -= essential_prime_implicants
        prime_implicants -= self._get_not_care_only_implicants(
            prime_implicants
        )
        return essential_prime_implicants, prime_implicants

    def _get_prime_implicants(self) -> Set[Cube]:
        """
        Quine-McCluskey with hash-based neighbor lookup: O(n * m) per level.
        Two cubes merge only if they share the mask and differ in exactly one
        fixed bit.
        """
        cubes = {(v, 0) for v in self._values}
        primes = set()
        while cubes:
            merged = set()
            combined = set()
            for value, mask in cubes:
                for b in range(self._inputs_number):
                    bit = 1 << b
                    if (mask | value) & bit:
                        continue
                    neighbor = (value | bit, mask)
                    if neighbor in cubes:
                        merged.add((value, mask | bit))
                        combined.add((value, mask))
                        combined.add(neighbor)
            primes |= cubes - combined
            cubes = merged
        return {Cube(v, m) for v, m in primes}

    def _get_not_care_only_implicants(self, cubes: Set[Cube]) -> Set[Cube]:
        return {
            cube for cube in cubes
            if not any(cube.contains(one) for one in self._ones)
        }

    def _get_essential_prime_implicants(self, cubes: Set[Cube]) -> Set[Cube]:
        essential_prime_implicants = set()
        for one in self._ones:
            covering = [cube for cube in cubes if cube.contains(one)]
            if len(covering) == 1:
                essential_prime_implicants.add(covering[0])
        return essential_prime_implicants

    def get_all_solutions(self, max_solutions: int = 5) -> ReductionResult:
        """
        Exact minimum cover (fewest cubes). Returns up to ``max_solutions``
        minimal solutions, or all of them when ``max_solutions`` <= 0.
        """
        essentials, primes = self.minimize()
        pending = sorted(
            one for one in self._ones
            if not any(cube.contains(one) for cube in essentials)
        )
        # Bigger cubes first: good solutions show up early and tighten the bound
        candidates = sorted(
            (cube for cube in primes
             if any(cube.contains(one) for one in pending)),
            key=lambda c: (-c.mask.bit_count(), c.value, c.mask),
        )
        search = _CoverSearch(candidates, pending, max_solutions)
        n = self._inputs_number
        # Phase 1 may add one solution beyond max_solutions: keep the ones
        # with fewer literals
        solutions = sorted(
            (tuple(sorted(
                [*essentials, *cover],
                key=lambda c: c.to_pla(n),
            )) for cover in search.solve()),
            key=lambda s: (
                sum(c.literals(n) for c in s),
                [c.to_pla(n) for c in s],
            ),
        )
        if max_solutions > 0:
            solutions = solutions[:max_solutions]
        return ReductionResult(n, self._is_sop, solutions)


def _bits(mask: int) -> Iterator[int]:
    """Indices of the bits set in ``mask``, lowest first."""
    while mask:
        low = mask & -mask
        mask ^= low
        yield low.bit_length() - 1


class _CoverSearch:
    """
    Branch & bound over the cyclic core. Rows are the pending cells and
    columns the candidate cubes, both stored as bitmasks:
      - cover[i]: pending cells covered by cube i
      - covering[j]: cubes that cover pending cell j
    Every node is reduced (forced cubes, row and column dominance) before
    branching on the most constrained cell.

    Column dominance discards cubes that only lead to tied solutions, so the
    search runs twice: first with it to get the optimum size fast, then
    without it, bounded by that size, to enumerate the tied solutions. The
    second search can be much slower and only adds alternatives, so it stops
    after TIE_SEARCH_NODES nodes.
    """
    TIE_SEARCH_NODES = 5000

    def __init__(self, cubes: List[Cube], pending: List[int],
                 max_solutions: int):
        self._cubes = cubes
        self._dashes = [cube.mask.bit_count() for cube in cubes]
        self._cover = [
            sum(1 << j for j, cell in enumerate(pending) if cube.contains(cell))
            for cube in cubes
        ]
        self._covering = [
            sum(1 << i for i, cover in enumerate(self._cover) if cover >> j & 1)
            for j in range(len(pending))
        ]
        self._all_pending = (1 << len(pending)) - 1
        self._max_solutions = max_solutions
        self._solutions: List[List[int]] = []
        self._best_len = self._greedy_len()
        self._column_dominance = True
        self._nodes_left = -1

    def solve(self) -> List[List[Cube]]:
        all_cubes = (1 << len(self._cubes)) - 1
        max_solutions = self._max_solutions
        self._max_solutions = 1
        self._branch([], self._all_pending, all_cubes)
        found = {frozenset(sol) for sol in self._solutions}

        self._max_solutions = max_solutions
        self._column_dominance = False
        self._nodes_left = self.TIE_SEARCH_NODES
        self._solutions = []
        self._branch([], self._all_pending, all_cubes)
        found.update(frozenset(sol) for sol in self._solutions)
        return [[self._cubes[i] for i in sorted(sol)] for sol in found]

    def _greedy_len(self) -> int:
        """Size of a greedy cover: initial upper bound for the search."""
        pending = self._all_pending
        size = 0
        while pending:
            best = max(
                range(len(self._cubes)),
                key=lambda i: ((self._cover[i] & pending).bit_count(),
                               self._dashes[i]),
            )
            pending &= ~self._cover[best]
            size += 1
        return size

    def _is_full(self) -> bool:
        return 0 < self._max_solutions <= len(self._solutions)

    def _branch(self, chosen: List[int], pending: int, allowed: int):
        if self._nodes_left == 0:
            return
        self._nodes_left -= 1
        depth = len(chosen)
        pending, allowed, rows = self._reduce(chosen, pending, allowed)
        if rows is not None:
            if pending == 0:
                self._record(chosen)
            else:
                needed = len(chosen) + self._lower_bound(rows)
                if needed < self._best_len or (
                        needed == self._best_len and not self._is_full()):
                    best_row = min(rows, key=int.bit_count)
                    branch_cubes = sorted(
                        _bits(best_row),
                        key=lambda i: (-(self._cover[i] & pending).bit_count(),
                                       -self._dashes[i], i),
                    )
                    for i in branch_cubes:
                        chosen.append(i)
                        self._branch(chosen, pending & ~self._cover[i],
                                     allowed)
                        chosen.pop()
                        # Covers using cube i were already explored
                        allowed &= ~(1 << i)
        del chosen[depth:]

    def _reduce(self, chosen: List[int], pending: int, allowed: int):
        """
        Applies until nothing changes:
          - forced cubes: the only candidate left for some cell
          - row dominance: drop cell b if every cube covering some cell a
            also covers b (covering a covers b)
          - column dominance: drop cube p if a cube q covers a superset of
            its pending cells with no more literals; equal coverage with
            equal literals is a tie and both are kept
        Returns (pending, allowed, rows), with rows None when some cell has
        no candidate left.
        """
        while True:
            rows = {}
            for j in _bits(pending):
                row = self._covering[j] & allowed
                if row == 0:
                    return pending, allowed, None
                rows[j] = row

            forced = 0
            for row in rows.values():
                if row & (row - 1) == 0:
                    forced |= row
            if forced:
                for i in _bits(forced):
                    chosen.append(i)
                    pending &= ~self._cover[i]
                allowed &= ~forced
                continue

            dominated_cells = 0
            items = list(rows.items())
            for a, row_a in items:
                for b, row_b in items:
                    if a != b and row_a & ~row_b == 0 and (
                            row_a != row_b or a < b):
                        dominated_cells |= 1 << b
            if dominated_cells:
                pending &= ~dominated_cells
                continue

            columns = [(i, self._cover[i] & pending) for i in _bits(allowed)]
            dominated_cubes = 0
            for p, cover_p in columns:
                if cover_p == 0:
                    dominated_cubes |= 1 << p
                    continue
                if not self._column_dominance:
                    continue
                for q, cover_q in columns:
                    if p != q and cover_p & ~cover_q == 0 and (
                            self._dashes[q] > self._dashes[p] or (
                            self._dashes[q] == self._dashes[p]
                            and cover_p != cover_q)):
                        dominated_cubes |= 1 << p
                        break
            if dominated_cubes:
                allowed &= ~dominated_cubes
                continue

            return pending, allowed, list(rows.values())

    @staticmethod
    def _lower_bound(rows: List[int]) -> int:
        """
        Greedy independent set: cells that share no candidate cube each need
        a different cube, so their count is a valid lower bound.
        """
        used = 0
        bound = 0
        for row in sorted(rows, key=int.bit_count):
            if not row & used:
                used |= row
                bound += 1
        return bound

    def _record(self, chosen: List[int]):
        if len(chosen) < self._best_len:
            self._best_len = len(chosen)
            self._solutions = []
        if len(chosen) == self._best_len and not self._is_full():
            self._solutions.append(list(chosen))
