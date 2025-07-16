#include <iostream>
#include <chrono>
#include <iomanip>
#include <unordered_set>
#include <vector>
#include "reducer.hpp"

void getResults(const std::unordered_set<int>& ones, 
                const std::unordered_set<int>& not_care, 
                int input_number) {
    auto start = std::chrono::high_resolution_clock::now();
    Reducer reducer(input_number, ones, not_care, false);
    auto solutions = reducer.getAllSolutions();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double time_seconds = duration.count() / 1000000.0;
    std::cout << "\nResults" << std::endl;
    std::cout << "-----------------------" << std::endl;
    std::cout << "with ones {";
    bool first = true;
    for (int one : ones) {
        if (!first) std::cout << ", ";
        std::cout << one;
        first = false;
    }
    std::cout << "} and not_care {";
    first = true;
    for (int nc : not_care) {
        if (!first) std::cout << ", ";
        std::cout << nc;
        first = false;
    }
    std::cout << "}" << std::endl;
    std::cout << "times (s): " << std::fixed << std::setprecision(6) << time_seconds << std::endl;
    std::cout << "solution Reducer = {";
    first = true;
    for (const auto& solution : solutions) {
        if (!first) std::cout << ", ";
        std::cout << "{";
        bool inner_first = true;
        for (const auto& minterm : solution) {
            if (!inner_first) std::cout << ", ";
            std::cout << minterm.toString();
            inner_first = false;
        }
        std::cout << "}";
        first = false;
    }
    std::cout << "}" << std::endl;
}

int main() {
    // Casos de prueba equivalentes a main.py
    struct TestCase {
        std::unordered_set<int> ones;
        std::unordered_set<int> not_care;
        int input_number;
    };
    std::vector<TestCase> test_cases = {
        // 1
        {{0, 1, 2, 5, 6, 7, 8, 9, 10, 14}, {3, 11, 15}, 4},
        // 2
        {{}, {}, 3},
        // 3
        {{}, {0,1,2,3,4,5,6,7}, 3},
        // 4
        {{0,1,2,3,4,5,6,7}, {}, 3},
        // 5
        {{0,1,2,3,4,5,6}, {}, 3},
        // 6
        {{15}, {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14}, 4},
        // 7
        {{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14}, {15}, 4},
        // 8
        {{0, 1, 2, 4, 5, 6, 9, 10, 11, 13, 14, 15}, {}, 4},
        // 9 (codeproject)
        {{0, 2, 3, 5, 7, 8, 9, 10, 11, 13, 15}, {}, 4},
        // 10 (youtube)
        {{1,4,6,7,8,9,11,15}, {}, 4},
        // 11 (youtube)
        {{0, 1, 2, 11, 12, 13, 14}, {3, 7, 8}, 4},
        // 12 (youtube)
        {{1, 3, 4, 5, 9, 11, 12, 13, 14, 15}, {}, 4},
        // 13 (youtube)
        {{1, 3, 4, 5, 7, 9, 10, 11, 15}, {}, 4},
        // 14 (wiki Quine-McCluskey)
        {{4, 8, 10, 11, 12, 15}, {9, 14}, 4},
        // 15 (wiki Petrick)
        {{0, 1, 2, 5, 6, 7}, {}, 3},
        // 16 (electricaltechnology.org)
        {{0,2,8,9,10,12,13,16,18,24,25,26,29,31,32,34,35,39,40,42,43,47,48,50,56,58,61,63}, {}, 6}
    };
    for (const auto& test : test_cases) {
        getResults(test.ones, test.not_care, test.input_number);
    }
    return 0;
} 