// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <string>           // For std::string (e.g., identifier, name)
#include <vector>           // For storing epsilon values and possibly measurement data
#include "../observable.hpp" // Base class Observable, providing measurement framework
#include "../universe.hpp"   // Provides access to Universe’s geometry data (e.g., triangles)

// RicciDual class, inheriting from Observable to measure Ricci curvature in the dual lattice
class RicciDual : public Observable {
public:
    // Constructor: initializes the observable with an identifier and epsilon values
    // id: String identifier for output files (e.g., "collab-16000-1")
    // epsilons: Vector of integers specifying radii for dual sphere distance measurements
    RicciDual(std::string id, std::vector<int> epsilons)
        : Observable(id), epsilons(epsilons) {
        name = "ricci_dual";  // Set observable name for file naming and identification
    }

    // Implements the pure virtual process() method from Observable
    // Computes a Ricci curvature-like metric in the dual lattice across specified epsilon values
    // Likely uses average dual sphere distances to estimate curvature (Sec. 3.4)
    void process();

private:
    // Vector of epsilon values (radii) for dual sphere distance measurements
    // Passed via constructor, used in process() to compute curvature at multiple scales
    std::vector<int> epsilons;

    // Computes the average distance from a triangle to its epsilon-dual-sphere neighbors
    // t1: Starting triangle, epsilon: Radius for the dual sphere
    // Returns average dual link distance as a double, likely used in curvature calculation
    double averageSphereDistance(Triangle::Label t1, int epsilon);
};
