# KarnaughReducer
This is the algorithm for solve and reduce a K-Map

## Usage: Python Reducer

Requires Python 3.10 or newer (`int.bit_count`).

The `reducer.py` module provides a class `Reducer` to minimize boolean functions using the Quine-McCluskey algorithm. It finds the minimal set of prime implicants that cover all required minterms (ones), optionally considering don't-care conditions.

### How to Use

1. **Import the Reducer class:**
   ```python
   from reducer import Reducer
   ```
2. **Create a Reducer instance:**
   - `num_input`: Number of input variables (bits)
   - `ones`: Set of minterms where the function is 1
   - `not_care`: Set of minterms considered as don't-care (optional)
   ```python
   ones = {0, 1, 2, 5, 6, 7, 8, 9, 10, 14}
   not_care = {3, 11, 15}
   reducer = Reducer(num_input=4, ones=ones, not_care=not_care)
   ```
3. **Get all minimal solutions:**
   ```python
   result = reducer.get_all_solutions()  # max_solutions=5; <= 0 returns all
   print(result)
   ```
   The result is a `ReductionResult` with:
   - `is_sop`: whether the solutions are a Sum of Products or a Product of Sums. It applies to the whole result.
   - `solutions`: list of minimal solutions, each one a tuple of `Cube`.
   - `to_pla()`: the solutions as PLA strings (`'0-1-'`, with `-` for the free positions), the same notation Espresso uses.

   A `Cube` is a `(value, mask)` pair of ints: bits set in `mask` are the `-` positions. In SoP a cube groups ones; in PoS (`is_sop=False`) it groups zeros, so it is always the group drawn on the map. `cube.contains(cell)` is O(1) and `cube.cells()` enumerates only the cells of the group.

### How does it find the minimal solutions?

- **Prime implicants:** Quine-McCluskey with a hash-based neighbor lookup, O(n·m) per level instead of comparing every pair.
- **Essential prime implicants:** the only cube covering some cell.
- **Exact cover:** branch & bound over the remaining cells and cubes, stored as bitmasks. Every node is reduced (forced cubes, row and column dominance), branches on the cell with fewest candidates and prunes with an independent-set lower bound and a greedy upper bound.
- **Tied solutions:** column dominance discards cubes that only lead to tied solutions, so a second search without it enumerates the solutions of the optimum size, limited to `TIE_SEARCH_NODES` nodes. The optimum size is always exact; only the number of alternatives listed may be limited on hard functions.
- Solutions are sorted by number of literals.

### Example

```python
from reducer import Reducer

ones = {0, 1, 2, 5, 6, 7, 8, 9, 10, 14}
not_care = {3, 11, 15}
reducer = Reducer(num_input=4, ones=ones, not_care=not_care)
print(reducer.get_all_solutions())
```

### Output
```
SoP [['--1-', '-0--', '0--1']]
```

This means the minimal cover for the function is the set of terms: `--1-`, `-0--` and `0--1`.
