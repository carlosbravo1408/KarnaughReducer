#ifndef MINTERM_HPP
#define MINTERM_HPP

#include <string>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <iostream>

class Minterm {
private:
    std::string minterm_;
    std::unordered_set<int> ones_;
    bool is_sop_;

public:
    Minterm(const std::string& minterm, const std::unordered_set<int>& ones = {}, bool is_sop = true)
        : minterm_(minterm), ones_(ones), is_sop_(is_sop) {}

    const std::string& getMinterm() const { return minterm_; }
    const std::unordered_set<int>& getOnes() const { return ones_; }
    bool isSop() const { return is_sop_; }

    int hammingDistance(const Minterm& other) const {
        int distance = 0;
        for (size_t i = 0; i < minterm_.length(); ++i) {
            if (minterm_[i] != other.minterm_[i]) {
                distance++;
            }
        }
        return distance;
    }

    Minterm createImplicant(const Minterm& other) const {
        std::string result;
        std::unordered_set<int> combinedOnes = ones_;
        combinedOnes.insert(other.ones_.begin(), other.ones_.end());

        for (size_t i = 0; i < minterm_.length(); ++i) {
            if (minterm_[i] == other.minterm_[i]) {
                result += minterm_[i];
            } else {
                result += '-';
            }
        }

        return Minterm(result, combinedOnes);
    }

    Minterm sop2pos() const {
        int num_inputs = minterm_.length();
        std::string new_minterm_str;
        std::unordered_set<int> new_ones;
        
        // Invert the binary representation
        for (char c : minterm_) {
            if (c == '1') {
                new_minterm_str += '0';
            } else if (c == '0') {
                new_minterm_str += '1';
            } else {
                new_minterm_str += c; // Keep '-' as is
            }
        }
        
        // Create new ones set: all possible values minus current ones
        for (int i = 0; i < (1 << num_inputs); ++i) {
            new_ones.insert(i);
        }
        for (int one : ones_) {
            new_ones.erase(one);
        }
        
        return Minterm(new_minterm_str, new_ones, false);
    }

    bool operator==(const Minterm& other) const {
        return minterm_ == other.minterm_;
    }

    bool operator!=(const Minterm& other) const {
        return !(*this == other);
    }

    // Hash function for unordered containers
    struct Hash {
        std::size_t operator()(const Minterm& m) const {
            return std::hash<std::string>{}(m.minterm_);
        }
    };

    // String representation
    std::string toString() const {
        return minterm_ + ": is_sop=" + (is_sop_ ? "true" : "false");
    }

    // Stream operators for output
    friend std::ostream& operator<<(std::ostream& os, const Minterm& m) {
        os << m.toString();
        return os;
    }
};

#endif // MINTERM_HPP 