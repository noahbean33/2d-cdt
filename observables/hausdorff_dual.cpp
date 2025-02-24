// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include <string>               // For std::string and std::to_string
#include <vector>               // For storing dual sphere triangle labels
#include "hausdorff_dual.hpp"   // Header for HausdorffDual class, defining interface

// Implements the process() method to compute dual Hausdorff dimension
// Measures dual sphere sizes for increasing radii and formats results
void HausdorffDual::process() {
    std::string tmp = "";  // Temporary string to build the output

    // Set maximum epsilon to the number of time slices in the geometry
    // Represents the maximum dual distance to explore (Sec. 3.4)
    max_epsilon = Universe::nSlices;

    // Iterate over dual distances from 1 to max_epsilon - 1
    for (int i = 1; i < max_epsilon; i++) {
        auto t = randomTriangle();  // Select a random starting triangle (via Observable::randomTriangle())

        // Compute dual sphere at distance i from t
        std::vector<Triangle::Label> s1 = sphereDual(t, i);  // Uses Observable::sphereDual() for BFS

        // Append the number of triangles in the sphere to the output string
        tmp += std::to_string(s1.size());
        tmp += " ";  // Add space separator
    }
    tmp.pop_back();  // Remove trailing space

    output = tmp;  // Store formatted string in inherited output member
    // Output will be written by Observable::write() (e.g., "3 5 8 ...")
}
