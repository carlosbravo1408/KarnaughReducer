from typing import Set


class Minterm:
    def __init__(self, minterm: str, ones: Set):
        self._minterm = minterm
        self._ones = ones

    @property
    def minterm(self):
        return self._minterm

    @property
    def ones(self):
        return self._ones

    def __hash__(self):
        return hash(self._minterm)

    def hamming_distance(self, other: 'Minterm'):
        return sum(
            char1 != char2
            for char1, char2 in zip(self._minterm, other._minterm)
        )

    def create_implicant(self, other:'Minterm'):
        return Minterm(
            "".join(
                char1 if char1 == char2 else "-"
                for char1, char2 in zip(self._minterm, other._minterm)
            ),
            {*self._ones, *other._ones}
        )

    def __eq__(self, other: 'Minterm'):
        return self._minterm == other._minterm

    def __str__(self):
        return f"{self._minterm}"

    def __repr__(self):
        return f"{self._minterm}"
