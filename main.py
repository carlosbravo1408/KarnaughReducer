import time
import random

from reducer import Reducer

if __name__ == '__main__':
    start = time.time()
    ones, does_not_care, input_number = {0, 1, 2, 4, 5, 6, 9, 10, 11, 13, 14,
                                         15}, set(), 4
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    # https://www.codeproject.com/Articles/649849/A-Cplusplus-Karnaugh-Map-Minimizer-Infinite-Variab
    start = time.time()
    ones, does_not_care, input_number = {0, 2, 3, 5, 7, 8, 9, 10, 11, 13,
                                         15}, set(), 4
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    # https://www.youtube.com/watch?v=gpn3fvPU9Cs
    start = time.time()
    ones, does_not_care, input_number = {0, 1, 2, 11, 12, 13, 14}, {3, 7, 8}, 4
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    # # https://www.youtube.com/watch?v=DTOzK88Inkk
    start = time.time()
    ones, does_not_care, input_number = {1, 3, 4, 5, 9, 11, 12, 13, 14,
                                         15}, set(), 4
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    # # https://www.youtube.com/watch?v=_EMeZCdzX2k
    start = time.time()
    ones, does_not_care, input_number = {1, 3, 4, 5, 7, 9, 10, 11, 15}, set(), 4
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    # # https://en.wikipedia.org/wiki/Quine%E2%80%93McCluskey_algorithm
    start = time.time()
    ones, does_not_care, input_number = {4, 8, 10, 11, 12, 15}, {9, 14}, 4
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    # https://en.wikipedia.org/wiki/Petrick%27s_method
    start = time.time()
    ones, does_not_care, input_number = {0, 1, 2, 5, 6, 7}, set(), 3
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)

    input_number = 10
    ones = {random.randrange(
        0, 1 << input_number) for _ in range(1 << round(input_number * 0.7))}
    does_not_care = {random.randrange(
        0, 1 << input_number) for _ in range(1 << round(input_number * 0.4))}
    start = time.time()
    solver = Reducer(inputs_number=input_number, ones=ones,
                     not_care=does_not_care)
    solver.reduce()
    print(time.time() - start)
    print(solver.solutions)
