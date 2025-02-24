// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

/****
 * A Simplex<T> contains a single pool of objects of type T.
 * There exists only a single copy of a pool for a given type.
 * (Comment from original code, slightly rephrased for clarity)
 ****/

#include <cstdio>       // For standard I/O (e.g., printf in assertions)
#include <array>        // Unused here, possibly intended for future use
#include <cassert>      // For runtime assertions (e.g., pool integrity checks)
#include <random>       // Unused here, included for RNG in derived classes
#include <string>       // Used for std::to_string in some contexts
#include <typeinfo>     // Unused here, possibly for debugging type info

/****
 * Pool is a template class that maintains
 * a single, static pool (array) of objects of given class T.
 * The T class should inherit from Pool<T> to use this mechanism.
 * (Comment from original code, slightly expanded)
 ****/

template<class T>
class Pool {
private:
    // Static array holding all objects of type T (the pool itself)
    // Private to prevent external modification; managed via static methods
    static T *elements;

    // Index of the first free (inactive) cell in the pool
    // Points to the next available slot for allocation
    static int first;

    // Number of currently used (active) cells in the pool
    static int total;

    // Total capacity of the pool, set to T::pool_size at initialization
    static int capacity;

    // Instance-specific index of the next free entry or self-index when active
    // Negative when inactive (using ~ to mark), positive when active (self-referential)
    int next;

    // Friend declaration allows T (the inheriting class) to access private members
    // Ensures only T can construct itself via Pool’s mechanisms
    friend T;

    // Private default constructor to prevent instantiation outside T
    // Only T can create instances, enforcing the Pool<T> inheritance pattern
    Pool() = default;

protected:
    // Default pool size, must be overridden by child class (e.g., Vertex, Triangle)
    // -1 indicates it needs to be specialized; enforced by static_assert
    static const unsigned pool_size = -1;

public:
    // Deleted copy constructor to prevent copying Pool objects
    // Ensures pool objects are unique and managed solely via Pool
    Pool(const Pool&) = delete;

    // Deleted copy assignment operator for the same reason
    Pool& operator=(const Pool&) = delete;

    // Deleted move constructor to prevent moving Pool objects
    Pool(Pool&&) = delete;

    // Deleted move assignment operator for the same reason
    Pool& operator=(Pool&&) = delete;

    /**** Label ****
     * The nested Label type serves as a pointer to T,
     * but internally it is an integer index into the elements array.
     * Can be accessed as T::Label (e.g., Vertex::Label).
     ***************/
    class Label {
    private:
        // Index into the elements array representing a specific T object
        int i;  // Not const, allowing reassignment (e.g., in operators)

    public:
        // Default constructor, initializes i to undefined value
        Label() = default;

        // Constructor from integer, explicitly sets the index
        Label(int i) : i{i} { }

        // Dereference operator: returns reference to the T object at index i
        T& operator*() const { return T::at(i); }

        // Arrow operator: returns pointer to the T object at index i
        T* operator->() const { return &T::at(i); }

        // Conversion operator to int&: allows direct modification of i
        // Acts as a getter/setter for the index
        operator int&() { return i; }

        // Conversion operator to const int: allows read-only access to i
        operator int() const { return i; }
    };

    // Conversion operator to Label: returns this object’s index as a Label
    // Used when a Pool object is implicitly cast to its Label
    operator Label() const { return Label{next}; }

    // Creates and initializes the static pool of T objects
    // Called once to allocate the elements array
    static T* create_pool() {
        // Ensure child class has defined a valid pool_size
        static_assert(T::pool_size > 0, "Pool size not defined in child class");

        capacity = T::pool_size;  // Set capacity from child class’s pool_size
        elements = new T[capacity];  // Allocate array of T objects

        // Initialize all elements as free (inactive)
        // next is set to ~(i + 1), marking them as available with negative values
        for (auto i = 0; i < capacity; i++)
            elements[i].next = ~(i + 1);  // ~x = -(x + 1), avoids negative zero issue

        return elements;  // Return pointer to the allocated pool
    }

    // Allocates a new T object from the pool
    // Returns its Label (index) and marks it as active
    static Label create() {
        auto tmp = first;  // Get index of first free cell
        assert(elements[tmp].next < 0);  // Verify it’s inactive (negative next)
        first = ~elements[tmp].next;  // Update first to next free index
        elements[tmp].next = tmp;  // Mark as active by setting next to self
        total++;  // Increment active count
        return tmp;  // Return index as Label (implicit constructor)
    }

    // Deallocates a T object, returning it to the free pool
    // i: Label (index) of the object to destroy
    static void destroy(Label i) {
        elements[i].next = ~first;  // Mark as inactive, linking to previous first
        first = i;  // Set as new first free cell
        total--;  // Decrement active count
    }

    // Returns reference to the T object at index i in the pool
    static T& at(int i) { return elements[i]; }

    // Returns the number of currently active objects in the pool
    // noexcept: Guarantees no exceptions for performance
    static int size() noexcept { return total; }

    // Returns the total capacity of the pool
    static int pool_capacity() noexcept { return capacity; }

    //// Checks if the object is indeed in the right position in array 'elements' ////
    // Verifies this object’s index matches its position in the pool
    void check_in_pool() {
        assert(this->next >= 0);  // Must be active (non-negative next)
        assert(this->next < capacity);  // Index within bounds
        assert(this == elements + this->next);  // Pointer matches index
    }

    // Destroys this object, returning it to the free pool
    // Calls static destroy() after validation
    void destroy() {
        check_in_pool();  // Ensure object is valid
        destroy(this->next);  // Deallocate using its index
    }

    //// Pool iterator - even though there are no pool objects ////
    // Defines an iterator over active objects in the pool
    struct Iterator {
    private:
        int i;    // Current index in elements array
        int cnt;  // Count of active objects traversed

    public:
        // Constructor: initializes index and count
        Iterator(int i = 0, int cnt = 0) : i{i}, cnt{cnt} {}

        // Dereference operator: returns reference to current T object
        T& operator*() { return elements[i]; }

        // Equality operator: compares iteration count
        bool operator==(const Iterator& b) const { return cnt == b.cnt; }

        // Inequality operator: negates equality
        bool operator!=(const Iterator& b) const { return !operator==(b); }

        // Pre-increment operator: advances to next active object
        Iterator& operator++() {
            if (cnt < total - 1)  // If not at the last active object
                while (elements[++i].next < 0) continue;  // Skip inactive slots
            cnt++;  // Increment count
            return *this;
        }
    };

    // Helper struct providing begin() and end() for iteration
    struct Items {
        // Returns iterator to the first active object
        auto begin() {
            int i;
            for (i = 0; elements[i].next < 0; i++) continue;  // Find first active
            return Iterator{i, 0};
        }

        // Returns iterator to the end (past last active object)
        auto end() {
            return Iterator{-1, total};
        }
    };

    // Returns an Items object for range-based iteration over active objects
    static Items items() { return Items{}; }
};

// Static member initializations (outside class definition)
// Allocate and initialize the pool at program start
template<class T> T* Pool<T>::elements = Pool<T>::create_pool();
// Initialize first free index to 0
template<class T> int Pool<T>::first{0};
// Initialize active object count to 0
template<class T> int Pool<T>::total{0};
// Initialize capacity (set by create_pool())
template<class T> int Pool<T>::capacity;
