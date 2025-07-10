# KarnaughReducer
This is the algorithm for solve and reduce a K-Map

## Usage: Python Reducer

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
   solutions = reducer.get_all_solutions()
   print(solutions)
   ```
   Each solution is a set of minterms (as strings with '-' for don't-care positions) that together cover all required ones with the minimal number of terms.

### How does it estimate the shortest configuration?

- The algorithm first finds all **prime implicants** (possible groupings of minterms).
- It identifies **essential prime implicants** (those that are the only cover for some ones).
- If no essential prime implicants exist, it explores all combinations of prime implicants using a search (Dijkstra-like) to cover all ones.
- **Heuristic:** The search prioritizes nodes that cover more ones, but always keeps track of the shortest solution found so far.
- **Key logic:**
  - When a new, shorter solution is found, the set of solutions is reset to only keep those of minimal length.
  - Only solutions of minimal length are returned, ensuring all returned solutions are optimal.

### Example

```python
from reducer import Reducer

ones = {0, 1, 2, 5, 6, 7, 8, 9, 10, 14}
not_care = {3, 11, 15}
reducer = Reducer(num_input=4, ones=ones, not_care=not_care)
solutions = reducer.get_all_solutions()
print(solutions)
```

### Output
```
{frozenset({'0--1', '--1-', '-0--'})}
```

This means the minimal cover for the function is the set of terms: `0--1`, `--1-`, and `-0--`.
