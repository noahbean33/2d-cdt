// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include <unordered_map>        // For efficient triangle lookup in averageSphereDistance
#include <string>               // For std::string and std::to_string
#include <vector>               // For storing epsilon values, origins, and distances
#include <algorithm>            // For std::find (and std::accumulate, already used)
#include "ricci_dual.hpp"       // Header for RicciDual class, defining interface

// Implements the process() method to compute dual Ricci curvature
// Measures average dual sphere distances for each epsilon and formats results
void RicciDual::process() {
    std::vector<double> epsilonDistanceList;  // Stores average distances for each epsilon
    std::vector<Triangle::Label> origins;     // Starting triangles for each epsilon

    // Select a random origin triangle for each epsilon value from trianglesAll
    for (std::vector<int>::iterator it = epsilons.begin(); it != epsilons.end(); it++) {
        origins.push_back(Universe::trianglesAll.pick());  // Uses Bag’s random pick method
    }

    // Compute average dual sphere distance for each epsilon
    for (int i = 0; i < epsilons.size(); i++) {
        int epsilon = epsilons[i];          // Current epsilon (radius)
        // printf("%d - ", epsilon);       // Commented debug output for epsilon

        auto origin = origins[i];           // Corresponding origin triangle

        // Calculate average distance for this epsilon in the dual lattice
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

// Computes the average distance from a triangle’s epsilon-dual-sphere to another’s
// t1: Starting triangle, epsilon: Radius for dual sphere measurement
// Returns average dual link distance as a double, used in dual curvature estimation
double RicciDual::averageSphereDistance(Triangle::Label t1, int epsilon) {
    auto s1 = sphereDual(t1, epsilon);  // Get triangles at epsilon dual distance from t1 (via Observable::sphereDual())
    std::uniform_int_distribution<> rv(0, s1.size() - 1);  // Random index generator for s1
    auto t2 = s1.at(rv(rng));           // Select a random triangle t2 from s1
    auto s2 = sphereDual(t2, epsilon);  // Get triangles at epsilon dual distance from t2
    std::unordered_map<int, Triangle::Label> triangleMap;  // Map for fast lookup of s2 triangles

    std::vector<int> distanceList;  // Stores distances from s1 triangles to s2

    // Compute distances from each triangle in s1 to s2 using BFS in the dual lattice
    for (auto b : s1) {
        triangleMap.clear();  // Reset map for this triangle
        for (auto v : s2) {   // Populate map with s2 triangles (note: v is misnamed, should be t)
            triangleMap[v] = v;
        }

        std::vector<Triangle::Label> done;       // Tracks visited triangles
        std::vector<Triangle::Label> thisDepth;  // Current depth’s triangles
        std::vector<Triangle::Label> nextDepth;  // Next depth’s triangles

        done.push_back(b);      // Mark starting triangle as visited
        thisDepth.push_back(b); // Start BFS from b

        // BFS up to 3*epsilon depth (heuristic bound for curvature calculation)
        for (int currentDepth = 0; currentDepth < 3 * epsilon; currentDepth++) {
            for (auto v : thisDepth) {  // Explore neighbors at current depth (v should be t)
                // Check if current triangle is in s2
                if (triangleMap.find(v) != triangleMap.end()) {
                    distanceList.push_back(0);  // Distance 0 if v is in s2 (same triangle)
                    triangleMap.erase(v);       // Remove from map
                }
                // Explore neighbors in the dual lattice
                for (auto neighbor : Universe::triangleNeighbors[v]) {
                    if (std::find(done.begin(), done.end(), neighbor) == done.end()) {  // If unvisited
                        nextDepth.push_back(neighbor);  // Add to next depth
                        done.push_back(neighbor);       // Mark as visited

                        // Check if neighbor is in s2
                        if (triangleMap.find(neighbor) != triangleMap.end()) {
                            distanceList.push_back(currentDepth + 1);  // Record distance
                            triangleMap.erase(neighbor);               // Remove from map
                        }
                    }
                    if (triangleMap.size() == 0) break;  // Exit if all s2 triangles found
                }
                if (triangleMap.size() == 0) break;  // Exit inner loop if done
            }
            thisDepth = nextDepth;  // Move to next depth
            nextDepth.clear();      // Clear for next iteration
            if (triangleMap.size() == 0) break;  // Exit outer loop if done
        }
    }

    // Calculate average distance normalized by epsilon and number of distances
    int distanceSum = std::accumulate(distanceList.begin(), distanceList.end(), 0);
    double averageDistance = static_cast<double>(distanceSum) /
                             static_cast<double>(epsilon * distanceList.size());

    return averageDistance;  // Return average distance for dual curvature estimation
}
