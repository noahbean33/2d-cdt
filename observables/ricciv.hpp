// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <string>           // For std::string (e.g., identifier, name)
#include <vector>           // For storing epsilon values and possibly measurement data
#include "../observable.hpp" // Base class Observable, providing measurement framework
#include "../universe.hpp"   // Provides access to Universe’s geometry data (e.g., vertices)

// RicciV class, inheriting from Observable to measure vertical Ricci curvature
class RicciV : public Observable {
public:
    // Constructor: initializes the observable with an identifier and epsilon values
    // id: String identifier for output files (e.g., "collab-16000-1")
    // epsilons: Vector of integers specifying radii for sphere distance measurements
    RicciV(std::string id, std::vector<int> epsilons)
        : Observable(id), epsilons(epsilons) {
        name = "ricciv";  // Set observable name for file naming and identification
    }

    // Implements the pure virtual process() method from Observable
    // Computes vertical Ricci curvature-like metric across specified epsilon values
    // Likely uses average sphere distances to estimate curvature (Sec. 3.4)
    void process();

private:
    // Vector of epsilon values (radii) for sphere distance measurements
    // Passed via constructor, used in process() to compute curvature at multiple scales
    std::vector<int> epsilons;

    // Computes the average distance from a vertex to its epsilon-sphere neighbors
    // p1: Starting vertex, epsilon: Radius for the sphere
    // Returns average link distance as a double, likely used in curvature calculation
    double averageSphereDistance(Vertex::Label p1, int epsilon);
};
