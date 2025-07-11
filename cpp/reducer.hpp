#ifndef REDUCER_HPP
#define REDUCER_HPP

#include "minterm.hpp"
#include "node.hpp"
#include "priority_queue.hpp"
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <bitset>

// Helper template for hashing sets (moved before class definition)
template<typename T, typename Hash>
struct SetHash {
    std::size_t operator()(const std::unordered_set<T, Hash>& s) const {
        std::size_t hash = 0;
        for (const auto& item : s) {
            hash ^= Hash{}(item) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

class Reducer {
private:
    std::unordered_set<int> ones_;
    std::unordered_set<int> not_care_;
    std::unordered_set<int> values_;
    int inputs_number_;
    bool is_sop_;

public:
    Reducer(int num_input, const std::unordered_set<int>& ones = {}, 
            const std::unordered_set<int>& not_care = {}, bool is_sop = true)
        : ones_(ones), not_care_(not_care), inputs_number_(num_input), is_sop_(is_sop) {
        
        // Combine ones and don't care values
        values_.insert(ones_.begin(), ones_.end());
        values_.insert(not_care_.begin(), not_care_.end());
        
        if (!is_sop_) {
            invertInput();
        }
    }

    std::pair<std::unordered_set<Minterm, Minterm::Hash>, 
              std::unordered_set<Minterm, Minterm::Hash>> minimize() {
        
        auto prime_implicants = getPrimeImplicants();
        auto essential_prime_implicants = getEssentialPrimeImplicants(prime_implicants);
        
        // Remove essential prime implicants from prime implicants
        for (const auto& essential : essential_prime_implicants) {
            prime_implicants.erase(essential);
        }
        
        // Remove non-prime implicants
        auto non_prime_implicants = getNonPrimeImplicants(prime_implicants);
        for (const auto& non_prime : non_prime_implicants) {
            prime_implicants.erase(non_prime);
        }
        
        return {essential_prime_implicants, prime_implicants};
    }

    std::unordered_set<HashableSet<Minterm, Minterm::Hash>, HashableSet<Minterm, Minterm::Hash>::HashFunction> getAllSolutions(int max_solutions = 5) {
        
        auto [essential_prime_implicants, prime_implicants] = minimize();
        
        int shortest_solution_len = essential_prime_implicants.size() + prime_implicants.size();
        std::unordered_set<HashableSet<Minterm, Minterm::Hash>, 
                          HashableSet<Minterm, Minterm::Hash>::HashFunction> solutions;
        
        PriorityQueue<Node> frontier;
        std::unordered_set<HashableSet<Minterm, Minterm::Hash>, 
                          HashableSet<Minterm, Minterm::Hash>::HashFunction> configuration_visited;

        if (!essential_prime_implicants.empty()) {
            auto current_prime = *essential_prime_implicants.begin();
            essential_prime_implicants.erase(essential_prime_implicants.begin());
            
            auto start_node = std::make_shared<Node>(
                current_prime,
                nullptr,
                countMissingOnes(current_prime.getOnes())
            );

            while (!essential_prime_implicants.empty()) {
                current_prime = *essential_prime_implicants.begin();
                essential_prime_implicants.erase(essential_prime_implicants.begin());
                
                auto current_ones = start_node->getOnes();
                auto prime_ones = current_prime.getOnes();
                current_ones.insert(prime_ones.begin(), prime_ones.end());
                
                int cost = countMissingOnes(current_ones);
                start_node = std::make_shared<Node>(
                    current_prime,
                    start_node,
                    cost
                );
            }

                    if (countMissingOnes(start_node->getOnes()) == 0) {
            solutions.insert(start_node->getParents());
            if (!is_sop_) {
                solutions = sop2pos(solutions);
            }
            return solutions;
        }
            
            frontier.push(*start_node);
            configuration_visited.insert(start_node->getParents());
        } else {
            for (const auto& minterm : prime_implicants) {
                auto node = std::make_shared<Node>(
                    minterm,
                    nullptr,
                    countMissingOnes(minterm.getOnes())
                );
                frontier.push(*node);
                configuration_visited.insert(node->getParents());
            }
        }

        // Dijkstra algorithm
        while (!frontier.empty()) {
            Node current_node = frontier.pop();
            auto current_configuration = current_node.getParents();
            auto current_ones = current_node.getOnes();
            
            if (max_solutions > 0 && solutions.size() >= static_cast<size_t>(max_solutions)) {
                break;
            }
            
            if (current_configuration.size() > static_cast<size_t>(shortest_solution_len)) {
                continue;
            }
            
            for (const auto& minterm : prime_implicants) {
                if (minterm == current_node.getItem()) {
                    continue;
                }
                
                auto new_ones = current_ones;
                auto minterm_ones = minterm.getOnes();
                new_ones.insert(minterm_ones.begin(), minterm_ones.end());
                
                int cost = countMissingOnes(new_ones);
                auto node = std::make_shared<Node>(
                    minterm,
                    std::make_shared<Node>(current_node),
                    cost
                );
                
                auto node_configuration = node->getParents();
                
                if (configuration_visited.find(node_configuration) != configuration_visited.end()) {
                    continue;
                }
                
                if (current_configuration.contains(minterm)) {
                    continue;
                }
                
                if (cost >= current_node.getCost()) {
                    continue;
                }
                
                if (countMissingOnes(node->getOnes()) == 0) {
                    if (node_configuration.size() < static_cast<size_t>(shortest_solution_len)) {
                        shortest_solution_len = static_cast<int>(node_configuration.size());
                        solutions.clear(); // Reinicia el set de soluciones
                    }
                    if (node_configuration.size() <= static_cast<size_t>(shortest_solution_len)) {
                        solutions.insert(node_configuration);
                    }
                } else {
                    frontier.push(*node);
                }
                configuration_visited.insert(node_configuration);
            }
        }
        
        if (!is_sop_) {
            solutions = sop2pos(solutions);
        }
        
        return solutions;
    }

private:
    void invertInput() {
        std::unordered_set<int> all_values;
        for (int i = 0; i < (1 << inputs_number_); ++i) {
            all_values.insert(i);
        }
        
        // Remove ones and don't care from all values to get new ones
        for (int one : ones_) {
            all_values.erase(one);
        }
        for (int not_care : not_care_) {
            all_values.erase(not_care);
        }
        
        ones_ = all_values;
        values_.clear();
        values_.insert(ones_.begin(), ones_.end());
        values_.insert(not_care_.begin(), not_care_.end());
    }

    std::unordered_set<HashableSet<Minterm, Minterm::Hash>, 
                       HashableSet<Minterm, Minterm::Hash>::HashFunction> sop2pos(
            const std::unordered_set<HashableSet<Minterm, Minterm::Hash>, 
                                   HashableSet<Minterm, Minterm::Hash>::HashFunction>& solution) {
        std::unordered_set<HashableSet<Minterm, Minterm::Hash>, 
                          HashableSet<Minterm, Minterm::Hash>::HashFunction> new_solution;
        
        for (const auto& subset : solution) {
            HashableSet<Minterm, Minterm::Hash> pos_subset;
            for (const auto& minterm : subset) {
                pos_subset.insert(minterm.sop2pos());
            }
            new_solution.insert(pos_subset);
        }
        
        return new_solution;
    }

    std::unordered_set<Minterm, Minterm::Hash> getPrimeImplicants() {
        std::vector<Minterm> minterms;
        
        // Create initial minterms
        for (int v : values_) {
            std::string binary = intToBinary(v, inputs_number_);
            minterms.emplace_back(binary, std::unordered_set<int>{v});
        }
        
        int value = 0;
        while (value < inputs_number_ && minterms.size() > 1) {
            std::unordered_set<Minterm, Minterm::Hash> comparatives;
            std::vector<bool> visited(minterms.size(), false);
            
            for (size_t i = 0; i < minterms.size() - 1; ++i) {
                for (size_t j = i + 1; j < minterms.size(); ++j) {
                    if (minterms[i].hammingDistance(minterms[j]) == 1) {
                        visited[j] = visited[i] = true;
                        comparatives.insert(minterms[j].createImplicant(minterms[i]));
                    }
                }
            }
            
            for (size_t i = 0; i < visited.size(); ++i) {
                if (!visited[i]) {
                    comparatives.insert(minterms[i]);
                }
            }
            
            if (comparatives.size() == minterms.size()) {
                bool same = true;
                for (const auto& m : comparatives) {
                    if (std::find(minterms.begin(), minterms.end(), m) == minterms.end()) {
                        same = false;
                        break;
                    }
                }
                if (same) break;
            }
            
            value++;
            minterms.clear();
            minterms.insert(minterms.end(), comparatives.begin(), comparatives.end());
        }
        
        return std::unordered_set<Minterm, Minterm::Hash>(minterms.begin(), minterms.end());
    }

    std::unordered_set<Minterm, Minterm::Hash> getNonPrimeImplicants(
            const std::unordered_set<Minterm, Minterm::Hash>& minterms) {
        std::unordered_set<Minterm, Minterm::Hash> non_implicants;
        
        for (const auto& minterm : minterms) {
            auto minterm_ones = minterm.getOnes();
            bool has_intersection = false;
            for (int one : ones_) {
                if (minterm_ones.find(one) != minterm_ones.end()) {
                    has_intersection = true;
                    break;
                }
            }
            if (!has_intersection) {
                non_implicants.insert(minterm);
            }
        }
        
        return non_implicants;
    }

    std::unordered_set<Minterm, Minterm::Hash> getEssentialPrimeImplicants(
            const std::unordered_set<Minterm, Minterm::Hash>& minterms) {
        std::unordered_set<Minterm, Minterm::Hash> essential_prime_implicants;
        
        for (int one : ones_) {
            int cnt = 0;
            Minterm last_minterm("", {});
            bool found = false;
            
            for (const auto& minterm : minterms) {
                auto minterm_ones = minterm.getOnes();
                if (minterm_ones.find(one) != minterm_ones.end()) {
                    cnt++;
                    last_minterm = minterm;
                    found = true;
                    if (cnt > 1) break;
                }
            }
            
            if (cnt == 1 && found) {
                essential_prime_implicants.insert(last_minterm);
            }
        }
        
        return essential_prime_implicants;
    }

    int countMissingOnes(const std::unordered_set<int>& current_ones) const {
        int missing = 0;
        for (int one : ones_) {
            if (current_ones.find(one) == current_ones.end()) {
                missing++;
            }
        }
        return missing;
    }

    std::string intToBinary(int value, int width) const {
        std::stringstream ss;
        ss << std::setw(width) << std::setfill('0') << std::bitset<32>(value).to_string().substr(32 - width);
        return ss.str();
    }
};

#endif // REDUCER_HPP 