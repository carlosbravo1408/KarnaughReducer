from typing import Set


class Minterm:
    def __init__(self, minterm: str, ones: Set) -> None:
        self._minterm = minterm
        self._ones = ones
        self._is_sop = True
        self._num_inputs = len(self._minterm)

    @property
    def minterm(self) -> str:
        return self._minterm

    @property
    def ones(self) -> Set[int]:
        return self._ones

    @property
    def zeros(self) -> Set[int]:
        return {i for i in range(1 << self._num_inputs)} - self._ones

    @property
    def is_sop(self) -> bool:
        return self._is_sop

    def __hash__(self) -> int:
        return hash(self._minterm)

    def hamming_distance(self, other: 'Minterm') -> int:
        return sum(
            char1 != char2
            for char1, char2 in zip(self._minterm, other._minterm)
        )

    def create_implicant(self, other:'Minterm') -> 'Minterm':
        return Minterm(
            "".join(
                char1 if char1 == char2 else "-"
                for char1, char2 in zip(self._minterm, other._minterm)
            ),
            {*self._ones, *other._ones}
        )

    def sop2pos(self) -> 'Minterm':
        new_minterm_str = "".join(
            "0" if c == "1" else "1" if c == "0" else c for c in (self._minterm)
        )
        new_ones = self.zeros
        minterm = Minterm(new_minterm_str, new_ones)
        minterm._is_sop = False
        return minterm

    def __eq__(self, other: 'Minterm') -> bool:
        return self._minterm == other._minterm

    def __str__(self) -> str:
        return f"{self._minterm}: is_sop={self.is_sop}"

    def __repr__(self) -> str:
        return f"{self._minterm}: is_sop={self.is_sop}"
