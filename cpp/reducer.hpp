#ifndef REDUCER_HPP
#define REDUCER_HPP

#include "cover_search.hpp"
#include "cube.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

// Exact two-level minimizer: Quine-McCluskey prime implicants, essential
// primes and an exact minimum cover of the remaining cells (CoverSearch).
class Reducer {
public:
    // The prime implicant table holds one bit per possible cube: 4^n bits
    static constexpr int kMaxInputs = 12;

    // In PoS (is_sop = false) the cells to cover are the zeros
    Reducer(int num_inputs, const std::vector<int>& ones, const std::vector<int>& not_care = {},
            bool is_sop = true)
        : num_inputs_(num_inputs) {
        if (num_inputs < 0 || num_inputs > kMaxInputs) {
            throw std::invalid_argument("num_inputs must be between 0 and 12");
        }
        cells_.assign(size_t{1} << num_inputs, is_sop ? Cell::Other : Cell::Target);
        for (int one : ones) {
            cells_.at(one) = is_sop ? Cell::Target : Cell::Other;
        }
        for (int cell : not_care) {
            cells_.at(cell) = Cell::NotCare;
        }
    }

    // Up to max_solutions minimal solutions (fewest cubes, sorted by
    // literals), or all of them when max_solutions <= 0. Every cube groups
    // ones in SoP and zeros in PoS; that applies to the whole result, not to
    // each cube.
    std::vector<std::vector<Cube>> getAllSolutions(int max_solutions = 5) const {
        std::vector<Cube> primes = primeImplicants();
        std::vector<bool> essential = essentialPrimeImplicants(primes);

        std::vector<Cube> essentials;
        std::vector<char> covered(cells_.size(), 0);
        for (size_t i = 0; i < primes.size(); ++i) {
            if (essential[i]) {
                essentials.push_back(primes[i]);
                primes[i].forEachCell([&](uint32_t cell) { covered[cell] = 1; });
            }
        }
        std::vector<uint32_t> pending;
        for (uint32_t cell = 0; cell < cells_.size(); ++cell) {
            if (cells_[cell] == Cell::Target && !covered[cell]) {
                pending.push_back(cell);
            }
        }

        std::vector<Cube> candidates;
        for (size_t i = 0; i < primes.size(); ++i) {
            bool covers_pending = false;
            primes[i].forEachCell([&](uint32_t cell) {
                covers_pending |= cells_[cell] == Cell::Target && !covered[cell];
            });
            if (!essential[i] && covers_pending) {
                candidates.push_back(primes[i]);
            }
        }
        // Bigger cubes first: good solutions show up early and tighten the bound
        std::sort(candidates.begin(), candidates.end(), [](Cube a, Cube b) {
            if (a.dashes() != b.dashes()) {
                return a.dashes() > b.dashes();
            }
            return a.value != b.value ? a.value < b.value : a.mask < b.mask;
        });

        std::vector<std::vector<Cube>> solutions;
        for (const auto& cover : CoverSearch(candidates, pending, max_solutions).solve()) {
            std::vector<Cube> solution = essentials;
            for (int i : cover) {
                solution.push_back(candidates[i]);
            }
            std::sort(solution.begin(), solution.end());
            solutions.push_back(std::move(solution));
        }
        // Phase 1 of the search may add one solution beyond max_solutions: keep
        // the ones with fewer literals
        auto literals = [this](const std::vector<Cube>& solution) {
            int count = 0;
            for (Cube cube : solution) {
                count += cube.literals(num_inputs_);
            }
            return count;
        };
        std::sort(solutions.begin(), solutions.end(), [&](const auto& a, const auto& b) {
            int literals_a = literals(a);
            int literals_b = literals(b);
            if (literals_a != literals_b) {
                return literals_a < literals_b;
            }
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
        });
        if (max_solutions > 0 && solutions.size() > static_cast<size_t>(max_solutions)) {
            solutions.resize(max_solutions);
        }
        return solutions;
    }

private:
    enum class Cell : uint8_t { Other, Target, NotCare };

    int num_inputs_;
    std::vector<Cell> cells_;

    // Quine-McCluskey with neighbor lookup in a table of 4^n bits indexed by
    // (mask, value): O(n * m) per level. Two cubes merge only if they share the
    // mask and differ in exactly one fixed bit, so a neighbor is always in the
    // same level.
    std::vector<Cube> primeImplicants() const {
        const uint32_t all_inputs = (uint32_t{1} << num_inputs_) - 1;
        auto key = [this](Cube cube) { return (size_t{cube.mask} << num_inputs_) | cube.value; };
        const size_t table_bits = size_t{1} << (2 * num_inputs_);
        std::vector<uint64_t> present(bits::words(table_bits));
        std::vector<uint64_t> combined(bits::words(table_bits));

        std::vector<Cube> level;
        for (uint32_t cell = 0; cell < cells_.size(); ++cell) {
            if (cells_[cell] != Cell::Other) {
                level.push_back({cell, 0});
                bits::set(present, key(level.back()));
            }
        }
        std::vector<Cube> next;
        std::vector<Cube> primes;
        while (!level.empty()) {
            next.clear();
            for (Cube cube : level) {
                for (uint32_t free = all_inputs & ~(cube.value | cube.mask); free != 0; free &= free - 1) {
                    uint32_t bit = free & (~free + 1);
                    Cube neighbor{cube.value | bit, cube.mask};
                    if (!bits::test(present, key(neighbor))) {
                        continue;
                    }
                    bits::set(combined, key(cube));
                    bits::set(combined, key(neighbor));
                    Cube merged{cube.value, cube.mask | bit};
                    if (!bits::test(present, key(merged))) {
                        bits::set(present, key(merged));
                        next.push_back(merged);
                    }
                }
            }
            for (Cube cube : level) {
                if (!bits::test(combined, key(cube))) {
                    primes.push_back(cube);
                }
            }
            level.swap(next);
        }
        return primes;
    }

    // essential[i] is true when primes[i] is the only prime covering some cell
    std::vector<bool> essentialPrimeImplicants(const std::vector<Cube>& primes) const {
        std::vector<uint32_t> count(cells_.size(), 0);
        std::vector<uint32_t> last(cells_.size(), 0);
        for (uint32_t i = 0; i < primes.size(); ++i) {
            primes[i].forEachCell([&](uint32_t cell) {
                ++count[cell];
                last[cell] = i;
            });
        }
        std::vector<bool> essential(primes.size(), false);
        for (uint32_t cell = 0; cell < cells_.size(); ++cell) {
            if (cells_[cell] == Cell::Target && count[cell] == 1) {
                essential[last[cell]] = true;
            }
        }
        return essential;
    }
};

#endif // REDUCER_HPP
