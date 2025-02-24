// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <string>           // For std::string (e.g., identifier, name)
#include "../observable.hpp" // Base class Observable, providing measurement framework
#include "../universe.hpp"   // Provides access to Universe’s geometry data (e.g., vertices)

// Hausdorff class, inheriting from Observable to measure primal Hausdorff dimension
class Hausdorff : public Observable {
public:
    // Constructor: initializes the observable with an identifier
    // id: String identifier for output files (e.g., "collab-16000-1")
    Hausdorff(std::string id) : Observable(id) {
        name = "hausdorff";  // Set observable name for file naming and identification
    }

    // Implements the pure virtual process() method from Observable
    // Computes the Hausdorff dimension by analyzing sphere growth in the primal lattice
    // Likely measures vertex counts at varying distances (Sec. 3.4)
    void process();

private:
    // Maximum epsilon (radius) for sphere measurements
    // Used to bound the distance range for Hausdorff dimension calculation
    int max_epsilon;
};
