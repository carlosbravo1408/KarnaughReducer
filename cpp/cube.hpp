#ifndef CUBE_HPP
#define CUBE_HPP

#include <bit>
#include <cstdint>
#include <string>
#include <string_view>

// Product term in positional (PLA) notation. Bits set in mask are the '-'
// positions; value holds the fixed bits and is 0 where masked.
// In a SoP solution the cube groups ones; in a PoS solution it groups zeros,
// so it is always the group drawn on the map.
struct Cube {
    uint32_t value = 0;
    uint32_t mask = 0;

    static Cube fromPla(std::string_view pla) {
        Cube cube;
        for (char c : pla) {
            cube.value <<= 1;
            cube.mask <<= 1;
            if (c == '1') {
                cube.value |= 1;
            } else if (c == '-') {
                cube.mask |= 1;
            }
        }
        return cube;
    }

    std::string toPla(int num_inputs) const {
        std::string pla(num_inputs, '0');
        for (int i = 0; i < num_inputs; ++i) {
            uint32_t bit = uint32_t{1} << (num_inputs - 1 - i);
            pla[i] = (mask & bit) ? '-' : (value & bit) ? '1' : '0';
        }
        return pla;
    }

    constexpr bool contains(uint32_t cell) const { return (cell & ~mask) == value; }

    constexpr int dashes() const { return std::popcount(mask); }

    constexpr int literals(int num_inputs) const { return num_inputs - dashes(); }

    // Calls f(cell) for every cell of the group, without walking the whole map
    template <typename F>
    void forEachCell(F&& f) const {
        uint32_t sub = mask;
        while (true) {
            f(value | sub);
            if (sub == 0) {
                return;
            }
            sub = (sub - 1) & mask;
        }
    }

    friend constexpr bool operator==(const Cube&, const Cube&) = default;

    // Ordered like its PLA string, with '-' < '0' < '1'
    friend constexpr bool operator<(Cube a, Cube b) {
        uint32_t diff = (a.value ^ b.value) | (a.mask ^ b.mask);
        if (diff == 0) {
            return false;
        }
        uint32_t bit = uint32_t{1} << (31 - std::countl_zero(diff));
        return a.rank(bit) < b.rank(bit);
    }

private:
    constexpr int rank(uint32_t bit) const { return (mask & bit) ? 0 : (value & bit) ? 2 : 1; }
};

#endif // CUBE_HPP
