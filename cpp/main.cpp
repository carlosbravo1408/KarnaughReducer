#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include "reducer.hpp"

void getResults(const std::vector<int>& ones, const std::vector<int>& not_care,
                int input_number, bool is_sop = true) {
    auto start = std::chrono::steady_clock::now();
    auto solutions = Reducer(input_number, ones, not_care, is_sop).getAllSolutions();
    std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    auto join = [](const std::vector<int>& values) {
        std::string text;
        for (int value : values) {
            text += (text.empty() ? "" : ", ") + std::to_string(value);
        }
        return text;
    };
    std::cout << "\nResults\n-----------------------\n";
    std::cout << "with ones {" << join(ones) << "} and not_care {" << join(not_care) << "}\n";
    std::cout << "times (s): " << std::fixed << std::setprecision(6) << elapsed.count() << "\n";
    std::cout << "solution Reducer = " << (is_sop ? "SoP" : "PoS") << " [";
    for (size_t s = 0; s < solutions.size(); ++s) {
        std::cout << (s ? ", [" : "[");
        for (size_t c = 0; c < solutions[s].size(); ++c) {
            std::cout << (c ? ", '" : "'") << solutions[s][c].toPla(input_number) << "'";
        }
        std::cout << "]";
    }
    std::cout << "]\n";
}

int main() {
    // Same cases as main.py
    struct TestCase {
        std::vector<int> ones;
        std::vector<int> not_care;
        int input_number;
    };
    std::vector<TestCase> test_cases = {
        {{0, 1, 2, 5, 6, 7, 8, 9, 10, 14}, {3, 11, 15}, 4},
        {{}, {}, 3},
        {{}, {0, 1, 2, 3, 4, 5, 6, 7}, 3},
        {{0, 1, 2, 3, 4, 5, 6, 7}, {}, 3},
        {{0, 1, 2, 3, 4, 5, 6}, {}, 3},
        {{15}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}, 4},
        {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}, {15}, 4},
        {{0, 1, 2, 4, 5, 6, 9, 10, 11, 13, 14, 15}, {}, 4},
        // https://www.codeproject.com/Articles/649849/A-Cplusplus-Karnaugh-Map-Minimizer-Infinite-Variab
        {{0, 2, 3, 5, 7, 8, 9, 10, 11, 13, 15}, {}, 4},
        // https://www.youtube.com/watch?v=bpAvLVeGBK0
        {{1, 4, 6, 7, 8, 9, 11, 15}, {}, 4},
        // https://www.youtube.com/watch?v=gpn3fvPU9Cs
        {{0, 1, 2, 11, 12, 13, 14}, {3, 7, 8}, 4},
        // https://www.youtube.com/watch?v=DTOzK88Inkk
        {{1, 3, 4, 5, 9, 11, 12, 13, 14, 15}, {}, 4},
        // https://www.youtube.com/watch?v=_EMeZCdzX2k
        {{1, 3, 4, 5, 7, 9, 10, 11, 15}, {}, 4},
        // https://en.wikipedia.org/wiki/Quine%E2%80%93McCluskey_algorithm
        {{4, 8, 10, 11, 12, 15}, {9, 14}, 4},
        // https://en.wikipedia.org/wiki/Petrick%27s_method
        {{0, 1, 2, 5, 6, 7}, {}, 3},
        // https://www.electricaltechnology.org/2018/05/karnaugh-map-k-map.html#5-6-variable-karnaugh-maps
        {{0, 2, 8, 9, 10, 12, 13, 16, 18, 24, 25, 26, 29, 31, 32, 34, 35, 39, 40, 42, 43, 47, 48,
          50, 56, 58, 61, 63}, {}, 6},
        // Minimum SoP has 8 terms (4 solutions); the former greedy search
        // returned 9 depending on the iteration order of the prime implicants.
        {{0, 2, 5, 7, 8, 10, 13, 17, 21, 23, 27, 29, 31, 32, 34, 40, 42, 44, 48, 50, 52, 53, 55,
          58, 59, 63}, {15, 16, 25, 39, 45}, 6},
    };
    for (const auto& test : test_cases) {
        getResults(test.ones, test.not_care, test.input_number);
    }
    return 0;
}
