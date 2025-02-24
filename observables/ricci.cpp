// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include <string>               // For std::string and std::to_string
#include <vector>               // For storing epsilon values, origins, and distances
#include <unordered_map>        // For efficient vertex lookup in averageSphereDistance
#include "ricci.hpp"            // Header for Ricci class, defining interface

// Implements the process() method to compute general Ricci curvature
// Measures average sphere distances for each epsilon and formats results
void Ricci::process() {
    std::vector<double> epsilonDistanceList;  // Stores average distances for each epsilon
    std::vector<Vertex::Label> origins;       // Starting vertices for each epsilon

    // Select a random origin vertex for each epsilon value
    for (std::vector<int>::iterator it = epsilons.begin(); it != epsilons.end(); it++) {
        origins.push_back(randomVertex());  // Uses Observable’s randomVertex()
    }

    // Compute average sphere distance for each epsilon
    for (int i = 0; i < epsilons.size(); i++) {
        int epsilon = epsilons[i];          // Current epsilon (radius)
        // printf("%d - ", epsilon);       // Commented debug output for epsilon

        auto origin = origins[i];           // Corresponding origin vertex

        // Calculate average distance for this epsilon in the primal lattice
        double averageDistance = averageSphereDistance(origin, epsilon);
        epsilonDistanceList.push_back(averageDistance);  // Store result

        // printf("%f\n", averageDistance);  // Commented debug output for distance
    }

    // Format results into a space-separated string
    std::string tmp = "";
    for (double dst : epsilonDistanceList) {
        tmp += std::to_string(dst);  // Append each distance as a string
        tmp += " ";                  // Add space separator
    }
    tmp.pop_back();  // Remove trailing space

    output = tmp;  // Store formatted string in inherited output member
    // Output will be written by Observable::write() (e.g., "2.5 3.0 ...")
}

// Computes the average distance from a vertex’s epsilon-sphere to another’s
// p1: Starting vertex, epsilon: Radius for sphere measurement
// Returns average link distance as a double, used in general curvature estimation
double Ricci::averageSphereDistance(Vertex::Label p1, int epsilon) {
    auto s1 = sphere(p1, epsilon);  // Get vertices at epsilon distance from p1 (via Observable::sphere())
    std::uniform_int_distribution<> rv(0, s1.size() - 1);  // Random index generator for s1
    auto p2 = s1.at(rv(rng));       // Select a random vertex p2 from s1
    auto s2 = sphere(p2, epsilon);  // Get vertices at epsilon distance from p2
    std::unordered_map<int, Vertex::Label> vertexMap;  // Map for fast lookup of s2 vertices

    std::vector<int> distanceList;  // Stores distances from s1 vertices to s2

    // Compute distances from each vertex in s1 to s2 using BFS in the primal lattice
    for (auto b : s1) {
        vertexMap.clear();  // Reset map for this vertex
        for (auto v : s2) {  // Populate map with s2 vertices
            vertexMap[v] = v;
        }

        std::vector<Vertex::Label> done;       // Tracks visited vertices
        std::vector<Vertex::Label> thisDepth;  // Current depth’s vertices
        std::vector<Vertex::Label> nextDepth;  // Next depth’s vertices

        done.push_back(b);      // Mark starting vertex as visited
        thisDepth.push_back(b); // Start BFS from b

        // BFS up to 3*epsilon depth (heuristic bound for curvature calculation)
        for (int currentDepth = 0; currentDepth < 3 * epsilon; currentDepth++) {
            for (auto v : thisDepth) {
                // Check if current vertex is in s2
                if (vertexMap.find(v) != vertexMap.end()) {
                    distanceList.push_back(0);  // Distance 0 if v is in s2 (same vertex)
                    vertexMap.erase(v);         // Remove from map
                }// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include <string>               // For std::string and std::to_string
#include <vector>               // For storing epsilon values, origins, and distances
#include <unordered_map>        // For efficient vertex lookup in averageSphereDistance
#include "ricci.hpp"            // Header for Ricci class, defining interface

// Implements the process() method to compute general Ricci curvature
// Measures average sphere distances for each epsilon and formats results
void Ricci::process() {
    std::vector<double> epsilonDistanceList;  // Stores average distances for each epsilon
    std::vector<Vertex::Label> origins;       // Starting vertices for each epsilon

    // Select a random origin vertex for each epsilon value
    for (std::vector<int>::iterator it = epsilons.begin(); it != epsilons.end(); it++) {
        origins.push_back(randomVertex());  // Uses Observable’s randomVertex()
    }

    // Compute average sphere distance for each epsilon
    for (int i = 0; i < epsilons.size(); i++) {
        int epsilon = epsilons[i];          // Current epsilon (radius)
        // printf("%d - ", epsilon);       // Commented debug output for epsilon

        auto origin = origins[i];           // Corresponding origin vertex

        // Calculate average distance for this epsilon in the primal lattice
        double averageDistance = averageSphereDistance(origin, epsilon);
        epsilonDistanceList.push_back(averageDistance);  // Store result

        // printf("%f\n", averageDistance);  // Commented debug output for distance
    }

    // Format results into a space-separated string
    std::string tmp = "";
    for (double dst : epsilonDistanceList) {
        tmp += std::to_string(dst);  // Append each distance as a string
        tmp += " ";                  // Add space separator
    }
    tmp.pop_back();  // Remove trailing space

    output = tmp;  // Store formatted string in inherited output member
    // Output will be written by Observable::write() (e.g., "2.5 3.0 ...")
}

// Computes the average distance from a vertex’s epsilon-sphere to another’s
// p1: Starting vertex, epsilon: Radius for sphere measurement
// Returns average link distance as a double, used in general curvature estimation
double Ricci::averageSphereDistance(Vertex::Label p1, int epsilon) {
    auto s1 = sphere(p1, epsilon);  // Get vertices at epsilon distance from p1 (via Observable::sphere())
    std::uniform_int_distribution<> rv(0, s1.size() - 1);  // Random index generator for s1
    auto p2 = s1.at(rv(rng));       // Select a random vertex p2 from s1
    auto s2 = sphere(p2, epsilon);  // Get vertices at epsilon distance from p2
    std::unordered_map<int, Vertex::Label> vertexMap;  // Map for fast lookup of s2 vertices

    std::vector<int> distanceList;  // Stores distances from s1 vertices to s2

    // Compute distances from each vertex in s1 to s2 using BFS in the primal lattice
    for (auto b : s1) {
        vertexMap.clear();  // Reset map for this vertex
        for (auto v : s2) {  // Populate map with s2 vertices
            vertexMap[v] = v;
        }

        std::vector<Vertex::Label> done;       // Tracks visited vertices
        std::vector<Vertex::Label> thisDepth;  // Current depth’s vertices
        std::vector<Vertex::Label> nextDepth;  // Next depth’s vertices

        done.push_back(b);      // Mark starting vertex as visited
        thisDepth.push_back(b); // Start BFS from b

        // BFS up to 3*epsilon depth (heuristic bound for curvature calculation)
        for (int currentDepth = 0; currentDepth < 3 * epsilon; currentDepth++) {
            for (auto v : thisDepth) {
                // Check if current vertex is in s2
                if (vertexMap.find(v) != vertexMap.end()) {
                    distanceList.push_back(0);  // Distance 0 if v is in s2 (same vertex)
                    vertexMap.erase(v);         // Remove from map
                }
                // Explore neighbors
                for (auto neighbor : Universe::vertexNeighbors[v]) {
                    if (std::find(done.begin(), done.end(), neighbor) == done.end()) {  // If unvisited
                        nextDepth.push_back(neighbor);  // Add to next depth
                        done.push_back(neighbor);       // Mark as visited

                        // Check if neighbor is in s2
                        if (vertexMap.find(neighbor) != vertexMap.end()) {
                            distanceList.push_back(currentDepth + 1);  // Record distance
                            vertexMap.erase(neighbor);                 // Remove from map
                        }
                    }
                    if (vertexMap.size() == 0) break;  // Exit if all s2 vertices found
                }
                if (vertexMap.size() == 0) break;  // Exit inner loop if done
            }
            thisDepth = nextDepth;  // Move to next depth
            nextDepth.clear();      // Clear for next iteration
            if (vertexMap.size() == 0) break;  // Exit outer loop if done
        }
    }

    // Calculate average distance normalized by epsilon and number of distances
    int distanceSum = std::accumulate(distanceList.begin(), distanceList.end(), 0);
    double averageDistance = static_cast<double>(distanceSum) /
                             static_cast<double>(epsilon * distanceList.size());

    return averageDistance;  // Return average distance for curvature estimation
}
