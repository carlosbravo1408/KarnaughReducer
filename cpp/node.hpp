#ifndef NODE_HPP
#define NODE_HPP

#include "minterm.hpp"
#include <memory>
#include <unordered_set>
#include <vector>
#include <algorithm>

// Hashable set wrapper (similar to Python's FrozenSet)
template<typename T, typename Hash>
class HashableSet {
private:
    std::unordered_set<T, Hash> set_;

public:
    HashableSet() = default;
    
    HashableSet(const std::unordered_set<T, Hash>& s) : set_(s) {}
    
    // Constructor from iterators
    template<typename Iterator>
    HashableSet(Iterator begin, Iterator end) : set_(begin, end) {}
    
    // Insert elements
    void insert(const T& item) {
        set_.insert(item);
    }
    
    // Find element
    typename std::unordered_set<T, Hash>::const_iterator find(const T& item) const {
        return set_.find(item);
    }
    
    // Check if element exists
    bool contains(const T& item) const {
        return set_.find(item) != set_.end();
    }
    
    // Get size
    size_t size() const {
        return set_.size();
    }
    
    // Check if empty
    bool empty() const {
        return set_.empty();
    }
    
    // Iterators
    typename std::unordered_set<T, Hash>::const_iterator begin() const {
        return set_.begin();
    }
    
    typename std::unordered_set<T, Hash>::const_iterator end() const {
        return set_.end();
    }
    
    // Equality operator
    bool operator==(const HashableSet& other) const {
        if (set_.size() != other.set_.size()) return false;
        for (const auto& item : set_) {
            if (other.set_.find(item) == other.set_.end()) {
                return false;
            }
        }
        return true;
    }
    
    // Inequality operator
    bool operator!=(const HashableSet& other) const {
        return !(*this == other);
    }
    
    // Hash function
    struct HashFunction {
        std::size_t operator()(const HashableSet& hs) const {
            std::size_t hash = 0;
            // Use a commutative hash function that's independent of insertion order
            for (const auto& item : hs.set_) {
                // Use addition instead of XOR for better distribution
                hash += Hash{}(item);
            }
            return hash;
        }
    };
};

class Node {
private:
    std::shared_ptr<Minterm> item_;
    std::shared_ptr<Node> parent_;
    int cost_;

public:
    Node(const Minterm& item, std::shared_ptr<Node> parent = nullptr, int cost = 0)
        : item_(std::make_shared<Minterm>(item)), parent_(parent), cost_(cost) {}

    int getCost() const { return cost_; }
    const Minterm& getItem() const { return *item_; }
    std::shared_ptr<Node> getParent() const { return parent_; }

    // Return hashable set of parents (similar to Python's FrozenSet)
    HashableSet<Minterm, Minterm::Hash> getParents() const {
        HashableSet<Minterm, Minterm::Hash> solution;
        solution.insert(*item_);
        
        std::shared_ptr<Node> current = parent_;
        while (current != nullptr) {
            solution.insert(current->getItem());
            current = current->getParent();
        }
        
        return solution;
    }

    std::unordered_set<int> getOnes() const {
        std::unordered_set<int> combinedOnes;
        
        const auto& currentOnes = item_->getOnes();
        combinedOnes.insert(currentOnes.begin(), currentOnes.end());
        
        std::shared_ptr<Node> current = parent_;
        while (current != nullptr) {
            const auto& parentOnes = current->getItem().getOnes();
            combinedOnes.insert(parentOnes.begin(), parentOnes.end());
            current = current->getParent();
        }
        
        return combinedOnes;
    }

    bool operator<(const Node& other) const {
        return cost_ < other.cost_;
    }

    bool operator>(const Node& other) const {
        return cost_ > other.cost_;
    }

    bool operator==(const Node& other) const {
        return *item_ == other.getItem();
    }

    bool operator!=(const Node& other) const {
        return !(*this == other);
    }

    struct Hash {
        std::size_t operator()(const Node& n) const {
            return Minterm::Hash{}(n.getItem());
        }
    };

    std::string toString() const {
        return item_->toString();
    }

    friend std::ostream& operator<<(std::ostream& os, const Node& n) {
        os << n.getItem();
        return os;
    }
};

#endif // NODE_HPP 