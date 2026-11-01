#ifndef COVER_SEARCH_HPP
#define COVER_SEARCH_HPP

#include "cube.hpp"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <deque>
#include <set>
#include <span>
#include <vector>

// Operations on bitsets stored as spans of 64-bit words
namespace bits {

using Words = std::span<uint64_t>;
using ConstWords = std::span<const uint64_t>;

inline size_t words(size_t bits) { return std::max<size_t>(1, (bits + 63) / 64); }

inline void set(Words b, size_t i) { b[i / 64] |= uint64_t{1} << (i % 64); }

inline void reset(Words b, size_t i) { b[i / 64] &= ~(uint64_t{1} << (i % 64)); }

inline bool test(ConstWords b, size_t i) { return (b[i / 64] >> (i % 64)) & 1; }

// Sets bits 0 to count - 1 and clears the rest
inline void setFirst(Words b, size_t count) {
    std::fill(b.begin(), b.end(), 0);
    std::fill_n(b.begin(), count / 64, ~uint64_t{0});
    if (count % 64) {
        b[count / 64] = (uint64_t{1} << (count % 64)) - 1;
    }
}

inline bool isEmpty(ConstWords b) {
    return std::all_of(b.begin(), b.end(), [](uint64_t w) { return w == 0; });
}

inline int count(ConstWords b) {
    int n = 0;
    for (uint64_t w : b) {
        n += std::popcount(w);
    }
    return n;
}

inline int countAnd(ConstWords a, ConstWords b) {
    int n = 0;
    for (size_t k = 0; k < a.size(); ++k) {
        n += std::popcount(a[k] & b[k]);
    }
    return n;
}

inline bool isSubset(ConstWords a, ConstWords b) {
    for (size_t k = 0; k < a.size(); ++k) {
        if (a[k] & ~b[k]) {
            return false;
        }
    }
    return true;
}

inline bool intersects(ConstWords a, ConstWords b) {
    for (size_t k = 0; k < a.size(); ++k) {
        if (a[k] & b[k]) {
            return true;
        }
    }
    return false;
}

inline void assignAnd(Words result, ConstWords a, ConstWords b) {
    for (size_t k = 0; k < result.size(); ++k) {
        result[k] = a[k] & b[k];
    }
}

inline void andNot(Words b, ConstWords other) {
    for (size_t k = 0; k < b.size(); ++k) {
        b[k] &= ~other[k];
    }
}

inline void orWith(Words b, ConstWords other) {
    for (size_t k = 0; k < b.size(); ++k) {
        b[k] |= other[k];
    }
}

// Index of the first set bit at or after from, or -1
inline int next(ConstWords b, size_t from) {
    size_t k = from / 64;
    if (k >= b.size()) {
        return -1;
    }
    uint64_t word = b[k] & (~uint64_t{0} << (from % 64));
    while (word == 0) {
        if (++k == b.size()) {
            return -1;
        }
        word = b[k];
    }
    return static_cast<int>(k * 64 + std::countr_zero(word));
}

} // namespace bits

// Branch & bound over the cyclic core. Rows are the pending cells and columns
// the candidate cubes, both stored as bitsets:
//   - cover(i): pending cells covered by cube i
//   - covering(j): cubes that cover pending cell j
// Every node is reduced (forced cubes, row and column dominance) before
// branching on the most constrained cell.
//
// Column dominance discards cubes that only lead to tied solutions, so the
// search runs twice: first with it to get the optimum size fast, then without
// it, bounded by that size, to enumerate the tied solutions. The second search
// can be much slower and only adds alternatives, so it stops after
// kTieSearchNodes nodes.
//
// The bitsets live in flat vectors allocated once: one Frame per depth for the
// state that must survive the recursion, and shared scratch buffers for the
// rows and columns of the node being reduced.
class CoverSearch {
public:
    static constexpr long long kTieSearchNodes = 5000;

    CoverSearch(const std::vector<Cube>& cubes, const std::vector<uint32_t>& cells, int max_solutions)
        : cube_count_(cubes.size()),
          cell_count_(cells.size()),
          cube_words_(bits::words(cube_count_)),
          cell_words_(bits::words(cell_count_)),
          max_solutions_(max_solutions),
          dashes_(cube_count_),
          cover_(cube_count_ * cell_words_),
          covering_(cell_count_ * cube_words_),
          rows_(cell_count_ * cube_words_),
          row_cell_(cell_count_),
          row_size_(cell_count_),
          columns_(cube_count_ * cell_words_),
          column_cube_(cube_count_),
          column_size_(cube_count_),
          dropped_cells_(cell_words_),
          dropped_cubes_(cube_words_),
          forced_(cube_words_),
          used_(cube_words_) {
        row_order_.reserve(cell_count_);
        for (size_t i = 0; i < cube_count_; ++i) {
            dashes_[i] = cubes[i].dashes();
            for (size_t j = 0; j < cell_count_; ++j) {
                if (cubes[i].contains(cells[j])) {
                    bits::set(cover(i), j);
                    bits::set(covering(j), i);
                }
            }
        }
        best_length_ = greedyLength();
    }

    // Minimum covers as sorted indices of the cubes, without duplicates
    std::vector<std::vector<int>> solve() {
        std::set<std::vector<int>> found;
        int max_solutions = max_solutions_;
        run(1, true, -1);
        found.insert(solutions_.begin(), solutions_.end());
        run(max_solutions, false, kTieSearchNodes);
        found.insert(solutions_.begin(), solutions_.end());
        return std::vector<std::vector<int>>(found.begin(), found.end());
    }

private:
    // State of one depth that its children must not overwrite
    struct Frame {
        Frame(size_t cell_words, size_t cube_words)
            : pending(cell_words), allowed(cube_words), branch_row(cube_words) {}

        std::vector<uint64_t> pending;
        std::vector<uint64_t> allowed;
        std::vector<uint64_t> branch_row;
        std::vector<uint64_t> branch_keys;
    };

    size_t cube_count_;
    size_t cell_count_;
    size_t cube_words_;
    size_t cell_words_;
    int max_solutions_;
    int best_length_ = 0;
    bool column_dominance_ = true;
    long long nodes_left_ = -1;
    std::vector<int> dashes_;
    std::vector<uint64_t> cover_;
    std::vector<uint64_t> covering_;
    // A deque keeps the references to the frames of the parents valid
    std::deque<Frame> frames_;
    std::vector<std::vector<int>> solutions_;

    // Scratch of the node being reduced; overwritten by the next recursion
    std::vector<uint64_t> rows_;
    std::vector<int> row_cell_;
    std::vector<int> row_size_;
    std::vector<uint64_t> row_order_;
    std::vector<uint64_t> columns_;
    std::vector<int> column_cube_;
    std::vector<int> column_size_;
    std::vector<uint64_t> dropped_cells_;
    std::vector<uint64_t> dropped_cubes_;
    std::vector<uint64_t> forced_;
    std::vector<uint64_t> used_;

    bits::Words cover(size_t cube) { return {cover_.data() + cube * cell_words_, cell_words_}; }
    bits::Words covering(size_t cell) { return {covering_.data() + cell * cube_words_, cube_words_}; }
    bits::Words row(size_t r) { return {rows_.data() + r * cube_words_, cube_words_}; }
    bits::Words column(size_t c) { return {columns_.data() + c * cell_words_, cell_words_}; }

    Frame& frameAt(size_t depth) {
        while (frames_.size() <= depth) {
            frames_.emplace_back(cell_words_, cube_words_);
        }
        return frames_[depth];
    }

    bool isFull() const {
        return max_solutions_ > 0 && solutions_.size() >= static_cast<size_t>(max_solutions_);
    }

    void run(int max_solutions, bool column_dominance, long long nodes) {
        max_solutions_ = max_solutions;
        column_dominance_ = column_dominance;
        nodes_left_ = nodes;
        solutions_.clear();
        Frame& root = frameAt(0);
        bits::setFirst(root.pending, cell_count_);
        bits::setFirst(root.allowed, cube_count_);
        std::vector<int> chosen;
        branch(chosen, 0);
    }

    // Size of a greedy cover: initial upper bound for the search
    int greedyLength() {
        std::vector<uint64_t> pending(cell_words_);
        bits::setFirst(pending, cell_count_);
        int size = 0;
        while (!bits::isEmpty(pending)) {
            size_t best = 0;
            int best_gain = -1;
            for (size_t i = 0; i < cube_count_; ++i) {
                int gain = bits::countAnd(cover(i), pending);
                if (gain > best_gain || (gain == best_gain && dashes_[i] > dashes_[best])) {
                    best = i;
                    best_gain = gain;
                }
            }
            bits::andNot(pending, cover(best));
            ++size;
        }
        return size;
    }

    // Explores the node whose pending cells and allowed cubes are in the frame
    // of depth. Cubes forced by the reduction are appended to chosen and
    // removed again before returning.
    void branch(std::vector<int>& chosen, size_t depth) {
        if (nodes_left_ == 0) {
            return;
        }
        --nodes_left_;
        Frame& frame = frames_[depth];
        size_t chosen_before = chosen.size();
        int row_count = reduce(chosen, frame.pending, frame.allowed);
        if (row_count >= 0) {
            if (bits::isEmpty(frame.pending)) {
                record(chosen);
            } else {
                int needed = static_cast<int>(chosen.size()) + lowerBound(row_count);
                if (needed < best_length_ || (needed == best_length_ && !isFull())) {
                    branchOnMostConstrainedCell(chosen, depth, row_count);
                }
            }
        }
        chosen.resize(chosen_before);
    }

    void branchOnMostConstrainedCell(std::vector<int>& chosen, size_t depth, int row_count) {
        Frame& frame = frames_[depth];
        int best_row = 0;
        for (int r = 1; r < row_count; ++r) {
            if (row_size_[r] < row_size_[best_row]) {
                best_row = r;
            }
        }
        // The scratch rows are overwritten by the children
        auto best = row(best_row);
        std::copy(best.begin(), best.end(), frame.branch_row.begin());

        // Most new cells first, then bigger cubes, then index
        frame.branch_keys.clear();
        for (int i = bits::next(frame.branch_row, 0); i >= 0; i = bits::next(frame.branch_row, i + 1)) {
            int gain = bits::countAnd(cover(i), frame.pending);
            frame.branch_keys.push_back((uint64_t(cell_count_ - gain) << 40)
                                        | (uint64_t(32 - dashes_[i]) << 32)
                                        | uint32_t(i));
        }
        std::sort(frame.branch_keys.begin(), frame.branch_keys.end());

        Frame& child = frameAt(depth + 1);
        for (uint64_t key : frame.branch_keys) {
            int i = static_cast<int>(key & 0xffffffffu);
            chosen.push_back(i);
            child.pending = frame.pending;
            bits::andNot(child.pending, cover(i));
            child.allowed = frame.allowed;
            branch(chosen, depth + 1);
            chosen.pop_back();
            // Covers using cube i were already explored
            bits::reset(frame.allowed, i);
        }
    }

    // Applies until nothing changes:
    //   - forced cubes: the only candidate left for some cell
    //   - row dominance: drop cell b if every cube covering some cell a also
    //     covers b (covering a covers b)
    //   - column dominance: drop cube p if a cube q covers a superset of its
    //     pending cells with no more literals; equal coverage with equal
    //     literals is a tie and both are kept
    // Leaves the rows of the pending cells in the scratch buffers and returns
    // how many there are, or -1 when some cell has no candidate left.
    int reduce(std::vector<int>& chosen, bits::Words pending, bits::Words allowed) {
        while (true) {
            int row_count = 0;
            for (int j = bits::next(pending, 0); j >= 0; j = bits::next(pending, j + 1)) {
                auto r = row(row_count);
                bits::assignAnd(r, covering(j), allowed);
                int size = bits::count(r);
                if (size == 0) {
                    return -1;
                }
                row_cell_[row_count] = j;
                row_size_[row_count] = size;
                ++row_count;
            }

            std::fill(forced_.begin(), forced_.end(), 0);
            bool any_forced = false;
            for (int r = 0; r < row_count; ++r) {
                if (row_size_[r] == 1) {
                    bits::orWith(forced_, row(r));
                    any_forced = true;
                }
            }
            if (any_forced) {
                for (int i = bits::next(forced_, 0); i >= 0; i = bits::next(forced_, i + 1)) {
                    chosen.push_back(i);
                    bits::andNot(pending, cover(i));
                }
                bits::andNot(allowed, forced_);
                continue;
            }

            std::fill(dropped_cells_.begin(), dropped_cells_.end(), 0);
            bool any_cell_dropped = false;
            for (int a = 0; a < row_count; ++a) {
                for (int b = 0; b < row_count; ++b) {
                    // A subset with the same size is an equal row: keep the first one
                    if (a == b || row_size_[a] > row_size_[b]
                        || (row_size_[a] == row_size_[b] && a > b)) {
                        continue;
                    }
                    if (bits::isSubset(row(a), row(b))) {
                        bits::set(dropped_cells_, row_cell_[b]);
                        any_cell_dropped = true;
                    }
                }
            }
            if (any_cell_dropped) {
                bits::andNot(pending, dropped_cells_);
                continue;
            }

            int column_count = 0;
            for (int i = bits::next(allowed, 0); i >= 0; i = bits::next(allowed, i + 1)) {
                auto c = column(column_count);
                bits::assignAnd(c, cover(i), pending);
                column_cube_[column_count] = i;
                column_size_[column_count] = bits::count(c);
                ++column_count;
            }
            std::fill(dropped_cubes_.begin(), dropped_cubes_.end(), 0);
            bool any_cube_dropped = false;
            for (int p = 0; p < column_count; ++p) {
                if (column_size_[p] == 0 || (column_dominance_ && isDominated(p, column_count))) {
                    bits::set(dropped_cubes_, column_cube_[p]);
                    any_cube_dropped = true;
                }
            }
            if (any_cube_dropped) {
                bits::andNot(allowed, dropped_cubes_);
                continue;
            }

            return row_count;
        }
    }

    bool isDominated(int p, int column_count) {
        int dashes_p = dashes_[column_cube_[p]];
        for (int q = 0; q < column_count; ++q) {
            if (q == p || column_size_[q] < column_size_[p]) {
                continue;
            }
            int dashes_q = dashes_[column_cube_[q]];
            // Being a superset, a different size means a different coverage
            bool better = dashes_q > dashes_p
                          || (dashes_q == dashes_p && column_size_[q] != column_size_[p]);
            if (better && bits::isSubset(column(p), column(q))) {
                return true;
            }
        }
        return false;
    }

    // Greedy independent set: cells that share no candidate cube each need a
    // different cube, so their count is a valid lower bound.
    int lowerBound(int row_count) {
        row_order_.clear();
        for (int r = 0; r < row_count; ++r) {
            row_order_.push_back((uint64_t(row_size_[r]) << 32) | uint32_t(r));
        }
        std::sort(row_order_.begin(), row_order_.end());

        std::fill(used_.begin(), used_.end(), 0);
        int bound = 0;
        for (uint64_t key : row_order_) {
            auto r = row(key & 0xffffffffu);
            if (!bits::intersects(r, used_)) {
                bits::orWith(used_, r);
                ++bound;
            }
        }
        return bound;
    }

    void record(const std::vector<int>& chosen) {
        int length = static_cast<int>(chosen.size());
        if (length < best_length_) {
            best_length_ = length;
            solutions_.clear();
        }
        if (length == best_length_ && !isFull()) {
            std::vector<int> solution = chosen;
            std::sort(solution.begin(), solution.end());
            solutions_.push_back(std::move(solution));
        }
    }
};

#endif // COVER_SEARCH_HPP
