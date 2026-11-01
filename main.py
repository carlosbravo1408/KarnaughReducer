import random
import timeit
from reducer import Reducer as R1

def get_results(ones, not_care, input_number, is_sop=True):
    s1 = R1(num_input=input_number, ones=ones, not_care=not_care, is_sop=is_sop
            )
    t1 = timeit.timeit(lambda: s1.get_all_solutions(), number=10)
    sol1 = s1.get_all_solutions()

    result = f"""
Results
-----------------------
with ones {ones} and not_care {not_care}
times (s) : {t1} 
solution Reducer = {sol1} 
"""
    print(result)

def get_benchmark_case(input_number, ones_ratio=0.6, not_care_ratio=0.1):
    """
    Deterministic random function (seeded by input_number): 60% ones and
    10% not care by default.
    """
    cells = list(range(1 << input_number))
    random.Random(input_number).shuffle(cells)
    ones_end = int(len(cells) * ones_ratio)
    not_care_end = int(len(cells) * (ones_ratio + not_care_ratio))
    ones = set(cells[:ones_end])
    not_care = set(cells[ones_end:not_care_end])
    return ones, not_care

def get_prime_implicants_benchmark(input_number, number=10):
    ones, not_care = get_benchmark_case(input_number)
    s1 = R1(num_input=input_number, ones=ones, not_care=not_care)
    t1 = timeit.timeit(lambda: s1._get_prime_implicants(), number=number)
    primes = s1._get_prime_implicants()
    print(
        f"n={input_number:2} ones={len(ones):4} not_care={len(not_care):4} "
        f"primes={len(primes):5} time (s) per run: {t1 / number:.6f}"
    )

if __name__ == '__main__':

    ones = {0, 1, 2, 5, 6, 7, 8, 9, 10, 14}
    not_care = {3, 11, 15}
    input_number = 4
    get_results(ones, not_care, input_number)

    ones = set()
    not_care = set()
    input_number = 3
    get_results(ones, not_care, input_number)

    ones = set()
    not_care = {0,1,2,3,4,5,6,7}
    input_number = 3
    get_results(ones, not_care, input_number)

    ones = {0,1,2,3,4,5,6,7}
    not_care = set()
    input_number = 3
    get_results(ones, not_care, input_number)

    ones = {0,1,2,3,4,5,6}
    not_care = set()
    input_number = 3
    get_results(ones, not_care, input_number)

    ones = {15}
    not_care = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14}
    input_number = 4
    get_results(ones, not_care, input_number)

    ones = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14}
    not_care = {15}
    input_number = 4
    get_results(ones, not_care, input_number)

    ones = {0, 1, 2, 4, 5, 6, 9, 10, 11, 13, 14, 15}
    not_care = set()
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://www.codeproject.com/Articles/649849/A-Cplusplus-Karnaugh-Map-Minimizer-Infinite-Variab
    ones = {0, 2, 3, 5, 7, 8, 9, 10, 11, 13, 15}
    not_care = set()
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://www.youtube.com/watch?v=bpAvLVeGBK0
    ones = {1,4,6,7,8,9,11,15}
    not_care = set()
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://www.youtube.com/watch?v=gpn3fvPU9Cs
    ones = {0, 1, 2, 11, 12, 13, 14}
    not_care = {3, 7, 8}
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://www.youtube.com/watch?v=DTOzK88Inkk
    ones = {1, 3, 4, 5, 9, 11, 12, 13, 14, 15}
    not_care = set()
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://www.youtube.com/watch?v=_EMeZCdzX2k
    ones = {1, 3, 4, 5, 7, 9, 10, 11, 15}
    not_care = set()
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://en.wikipedia.org/wiki/Quine%E2%80%93McCluskey_algorithm
    ones = {4, 8, 10, 11, 12, 15}
    not_care = {9, 14}
    input_number = 4
    get_results(ones, not_care, input_number)

    # https://en.wikipedia.org/wiki/Petrick%27s_method
    ones = {0, 1, 2, 5, 6, 7}
    not_care = set()
    input_number = 3
    get_results(ones, not_care, input_number)

    # https://www.electricaltechnology.org/2018/05/karnaugh-map-k-map.html#5-6-variable-karnaugh-maps
    ones = {0,2,8,9,10,12,13,16,18,24,25,26,29,31,32,34,35,39,40,42,43,47,48,
            50,56,58,61,63}
    not_care = set()
    input_number = 6
    get_results(ones, not_care, input_number)

    # Minimum SoP has 8 terms (4 solutions); the former greedy search
    # returned 9 depending on the iteration order of the prime implicants.
    ones = {0, 2, 5, 7, 8, 10, 13, 17, 21, 23, 27, 29, 31, 32, 34, 40, 42, 44,
            48, 50, 52, 53, 55, 58, 59, 63}
    not_care = {15, 16, 25, 39, 45}
    input_number = 6
    get_results(ones, not_care, input_number)

    # Benchmark of _get_prime_implicants (hash-based Quine-McCluskey)
    for input_number in (4, 6, 7, 8, 9, 10):
        get_prime_implicants_benchmark(input_number)
