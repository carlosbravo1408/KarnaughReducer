import timeit
from reducer import Reducer as R1

def get_results(ones, not_care, input_number):
    s1 = R1(num_input=input_number, ones=ones, not_care=not_care, is_sop=False
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
