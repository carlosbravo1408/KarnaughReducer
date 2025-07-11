#include <iostream>
#include <chrono>
#include <iomanip>
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
    int input_number;
    std::cin >> input_number;
    
    int ones_count;
    std::cin >> ones_count;
    std::unordered_set<int> ones;
    for (int i = 0; i < ones_count; ++i) {
        int val;
        std::cin >> val;
        ones.insert(val);
    }
    
    int not_care_count;
    std::cin >> not_care_count;
    std::unordered_set<int> not_care;
    for (int i = 0; i < not_care_count; ++i) {
        int val;
        std::cin >> val;
        not_care.insert(val);
    }
    
    getResults(ones, not_care, input_number);
    
    return 0;
} 