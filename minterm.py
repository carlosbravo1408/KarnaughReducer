from typing import Union


class Minterm:
    def __init__(self, minterm: str, ones: set):
        self.minterm = minterm
        self.ones = ones

    def __hash__(self):
        return hash(self.minterm)

    def __eq__(self, other: Union[str, 'Minterm']):
        if isinstance(other, Minterm):
            return self.minterm == other.minterm
        if isinstance(other, str):
            return self.minterm == other

    def __str__(self):
        return self.minterm

    def __repr__(self):
        return self.minterm