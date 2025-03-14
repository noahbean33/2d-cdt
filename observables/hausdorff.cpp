// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include <vector>               // For storing primal sphere vertex labels
#include <string>               // For std::string and std::to_string
#include "hausdorff.hpp"        // Header for Hausdorff class, defining interface
#include <algorithm>            // For std::find and std::accumulate

// Implements the process() method to compute primal Hausdorff dimension
// Measures sphere sizes for increasing radii and formats results
void Hausdorff::process() {
    std::string tmp = "";  // Temporary string to build the output

    // Set maximum epsilon to half the number of time slices
    // Limits sphere radius to half the geometry’s temporal extent (Sec. 3.4)
    max_epsilon = Universe::nSlices / 2;

    // Iterate over distances from 1 to max_epsilon - 1
    for (int i = 1; i < max_epsilon; i++) {
        auto v = randomVertex();  // Select a random starting vertex (via Observable::randomVertex())

        // Compute sphere at distance i from v in the primal lattice
        std::vector<Vertex::Label> s1 = sphere(v, i);  // Uses Observable::sphere() for BFS

        // Append the number of vertices in the sphere to the output string
        tmp += std::to_string(s1.size());
        tmp += " ";  // Add space separator
    }
    tmp.pop_back();  // Remove trailing space

    output = tmp;  // Store formatted string in inherited output member
    // Output will be written by Observable::write() (e.g., "3 5 8 ...")
}
