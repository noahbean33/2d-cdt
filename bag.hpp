// Copyright 2020 Joren Brunekreef and Andrzej Görlich
/****
 * Bag is an implementation of a set-like data structure.
 * It provides fast (O(1)) add, remove and pick operations.
 * It stores integer values less than N.
 * (Comment from original code, preserved as is)
 ****/
#pragma once    // Ensures this header is included only once during compilation

#include <cassert>      // For runtime assertions (e.g., checking bag state)
#include <random>       // For random number generation in pick()

// Template class Bag, parameterized by type T (e.g., Vertex, Triangle) and capacity N
template <class T, unsigned int N>  // N is the maximum capacity
class Bag {
    // Alias for T::Label, the integer index type used by Pool<T> (e.g., Vertex::Label)
    using Label = typename T::Label;

private:
    // Array mapping labels (indices in Pool<T>) to positions in elements
    // Contains "holes" (EMPTY) for unused slots; size N matches capacity
    std::array<int, N> indices;

    // Array of active labels, stored contiguously (no holes up to size_)
    // Size N, but only size_ elements are valid
    std::array<Label, N> elements;

    // Maximum number of elements Bag can hold (equals N)
    unsigned int capacity_;

    // Current number of active elements in the Bag
    unsigned int size_;

    // Reference to a random number generator for picking elements
    std::default_random_engine &rng;

    // Enum defining the EMPTY marker for unused slots
    // -1 indicates an inactive or invalid entry in indices or elements
    enum : int {
        EMPTY = -1  // Could be constexpr, but enum suffices here
    };

public:
    // Constructor: initializes Bag with a random engine reference
    // rng: Random number generator for pick() operation
    explicit Bag(std::default_random_engine &rng)
        : capacity_(N), size_(0), rng(rng) {
        indices.fill(EMPTY);  // Initialize all indices to EMPTY (-1)
    }

    // Returns the current number of active elements in the Bag
    // noexcept: Guarantees no exceptions for performance
    int size() const noexcept {
        return size_;
    }

    // Checks if a label is present in the Bag
    // obj: Label to check (implicitly converted to int)
    // Returns true if obj’s index in indices is not EMPTY
    bool contains(Label obj) const {
        return indices[obj] != EMPTY;  // Uses Label’s int conversion
    }

    // Adds a label to the Bag
    // obj: Label to add (must not already be present)
    // Places obj at the next free position in elements and updates indices
    void add(Label obj) {
        assert(!contains(obj));  // Ensure obj isn’t already in Bag (checked elsewhere in Universe)
        indices[obj] = size_;    // Map obj to its position in elements
        elements[size_] = obj;   // Store obj at the end of active elements
        size_++;                 // Increment active count
    }

    // Removes a label from the Bag
    // obj: Label to remove (must be present)
    // Moves the last element to obj’s position to maintain contiguity
    void remove(Label obj) {
        assert(contains(obj));  // Ensure obj is in Bag (checked elsewhere in Universe)
        size_--;               // Decrement active count

        auto index = indices[obj];  // Get obj’s position in elements
        auto last = elements[size_]; // Get the last active element

        elements[index] = last;  // Replace obj with last element
        elements[size_] = EMPTY; // Mark old last position as empty
        indices[last] = index;   // Update last element’s index
        indices[obj] = EMPTY;    // Mark obj’s slot as empty
    }

    // Randomly picks and returns a label from the Bag
    // Returns a random active element using uniform distribution
    Label pick() const {
        assert(size_ > 0);  // Ensure Bag isn’t empty
        std::uniform_int_distribution<> uniform(0, size_ - 1);  // Range over active elements
        return elements[uniform(rng)];  // Return randomly selected label
    }

    // Logs the current state of elements array for debugging
    // Prints indices and their corresponding labels up to size_
    void log() {
        printf("elements\n");
        for (int i = 0; i < size_; i++) {
            printf("%d: %d\n", i, elements[i]);  // Print position and label
        }
        printf("--\n");
    }

    //// Iterator for objects stored in a Bag ////
    // Returns pointer to the start of active elements for iteration
    auto begin() { return &elements[0]; }

    // Returns pointer to the end of active elements (past size_)
    auto end() { return &elements[size_]; }
};
