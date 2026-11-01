from typing import Iterator, NamedTuple


class Cube(NamedTuple):
    """
    Product term in positional (PLA) notation. Bits set in ``mask`` are the
    '-' positions; ``value`` holds the fixed bits and is 0 where masked.

    In a SoP solution the cube groups ones; in a PoS solution it groups zeros,
    so it is always the group drawn on the map.
    """
    value: int
    mask: int

    @classmethod
    def from_pla(cls, pla: str) -> 'Cube':
        value = mask = 0
        for char in pla:
            value <<= 1
            mask <<= 1
            if char == "1":
                value |= 1
            elif char == "-":
                mask |= 1
        return cls(value, mask)

    def to_pla(self, num_inputs: int) -> str:
        return "".join(
            "-" if self.mask >> b & 1 else "1" if self.value >> b & 1 else "0"
            for b in range(num_inputs - 1, -1, -1)
        )

    def contains(self, cell: int) -> bool:
        return (cell & ~self.mask) == self.value

    def cells(self) -> Iterator[int]:
        sub = self.mask
        while True:
            yield self.value | sub
            if sub == 0:
                return
            sub = (sub - 1) & self.mask

    def literals(self, num_inputs: int) -> int:
        return num_inputs - self.mask.bit_count()
