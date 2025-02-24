// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include <fstream>      // For file I/O operations (reading/writing output files)
#include <vector>       // For storing vertex/triangle labels in sphere and distance methods
#include <algorithm>    // Unused here, possibly intended for future sorting operations
#include "observable.hpp" // Header for Observable class, defining interface and base members

// Initialize static RNG for random selection (e.g., in randomVertex(), randomTriangle())
// Currently seeded with 0; TODO suggests proper seeding needed
std::default_random_engine Observable::rng(0);  // TODO(JorenB): seed properly

// Writes the computed observable data to a file
// Appends output string to a file named using data_dir, name, identifier, and extension
void Observable::write() {
    // Construct filename (e.g., "out/VolumeProfile-collab-16000-1.dat")
    std::string filename = data_dir + name + "-" + identifier + extension;

    // Check if file exists and is readable (logic seems inverted; good() is true if OK)
    std::ifstream infile(filename);
    if (!infile.good()) {  // If file doesn’t exist or isn’t readable, exit with error
        printf("output file deleted\n");  // Likely a debug message; behavior seems odd
        exit(1);
    }
    infile.close();

    // Open file in append mode to add new measurement data
    std::ofstream file;
    file.open(filename, std::ios::out | std::ios::app);

    assert(file.is_open());  // Ensure file opened successfully

    file << output << "\n";  // Write output string followed by newline
    file.close();            // Close file
}

// Clears the observable’s output file by truncating it
// Resets file to empty state for new simulation runs
void Observable::clear() {
    // Construct filename (same as write())
    std::string filename = data_dir + name + "-" + identifier + extension;

    // Open file in truncate mode to erase contents
    std::ofstream file;
    file.open(filename, std::ios::out | std::ios::trunc);

    assert(file.is_open());  // Ensure file opened successfully

    file.close();  // Close file (empty now)
}

// Computes a metric sphere of given radius around a vertex using BFS
// origin: Starting vertex, radius: Maximum link distance
// Returns vector of vertices at exactly radius hops away (Sec. 3.4)
std::vector<Vertex::Label> Observable::sphere(Vertex::Label origin, int radius) {
    std::vector<bool> done;              // Tracks visited vertices
    std::vector<Vertex::Label> thisDepth; // Current depth’s vertices
    std::vector<Vertex::Label> nextDepth; // Next depth’s vertices

    // Find maximum vertex label to size done array
    int vmax = 0;
    for (auto v : Universe::vertices) {
        if (v > vmax) vmax = v;
    }
    done.resize(vmax + 1, false);  // Initialize all as unvisited

    done.at(origin) = true;        // Mark origin as visited
    thisDepth.push_back(origin);   // Start BFS from origin

    std::vector<Vertex::Label> vertexList;  // Result: vertices at radius

    // Iterate through depths up to radius
    for (int currentDepth = 0; currentDepth < radius; currentDepth++) {
        for (auto v : thisDepth) {  // Explore neighbors at current depth
            for (auto neighbor : Universe::vertexNeighbors[v]) {
                if (!done.at(neighbor)) {  // If neighbor unvisited
                    nextDepth.push_back(neighbor);  // Add to next depth
                    done.at(neighbor) = true;       // Mark as visited
                    if (currentDepth == radius - 1)  // If at target radius
                        vertexList.push_back(neighbor);  // Add to result
                }
            }
        }
        thisDepth = nextDepth;  // Move to next depth
        nextDepth.clear();      // Clear for next iteration
    }

    return vertexList;  // Return vertices at radius
}

// Computes a dual metric sphere of given radius around a triangle using BFS
// origin: Starting triangle, radius: Maximum dual link distance
// Returns vector of triangles at exactly radius hops away (Sec. 3.4)
std::vector<Triangle::Label> Observable::sphereDual(Triangle::Label origin, int radius) {
    std::vector<bool> done;                // Tracks visited triangles
    std::vector<Triangle::Label> thisDepth; // Current depth’s triangles
    std::vector<Triangle::Label> nextDepth; // Next depth’s triangles

    // Find maximum triangle label to size done array
    int tmax = 0;
    for (auto t : Universe::triangles) {
        if (t > tmax) tmax = t;
    }
    done.resize(tmax + 1, false);  // Initialize all as unvisited

    done.at(origin) = true;        // Mark origin as visited
    thisDepth.push_back(origin);   // Start BFS from origin

    std::vector<Triangle::Label> triangleList;  // Result: triangles at radius

    // Iterate through depths up to radius
    for (int currentDepth = 0; currentDepth < radius; currentDepth++) {
        for (auto t : thisDepth) {  // Explore neighbors at current depth
            for (auto neighbor : Universe::triangleNeighbors[t]) {
                if (!done.at(neighbor)) {  // If neighbor unvisited
                    nextDepth.push_back(neighbor);  // Add to next depth
                    done.at(neighbor) = true;       // Mark as visited
                    if (currentDepth == radius - 1)  // If at target radius
                        triangleList.push_back(neighbor);  // Add to result
                }
            }
        }
        thisDepth = nextDepth;  // Move to next depth
        nextDepth.clear();      // Clear for next iteration
    }

    return triangleList;  // Return triangles at radius
}

// Calculates the shortest link distance between two vertices using BFS
// v1, v2: Vertices to measure distance between
// Returns number of hops or -1 if unreachable (Sec. 3.4)
int Observable::distance(Vertex::Label v1, Vertex::Label v2) {
    if (v1 == v2) return 0;  // Same vertex: distance is 0

    std::vector<bool> done;              // Tracks visited vertices
    std::vector<Vertex::Label> thisDepth; // Current depth’s vertices
    std::vector<Vertex::Label> nextDepth; // Next depth’s vertices

    // Find maximum vertex label to size done array
    int vmax = 0;
    for (auto v : Universe::vertices) {
        if (v > vmax) vmax = v;
    }
    done.resize(vmax + 1, false);  // Initialize all as unvisited

    done.at(v1) = true;        // Mark start vertex as visited
    thisDepth.push_back(v1);   // Start BFS from v1

    int currentDepth = 0;      // Track depth (distance)
    do {
        for (auto v : thisDepth) {  // Explore neighbors at current depth
            for (auto neighbor : Universe::vertexNeighbors[v]) {
                if (neighbor == v2) return currentDepth + 1;  // Found target: return distance
                if (!done.at(neighbor)) {  // If neighbor unvisited
                    nextDepth.push_back(neighbor);  // Add to next depth
                    done.at(neighbor) = true;       // Mark as visited
                }
            }
        }
        thisDepth = nextDepth;  // Move to next depth
        nextDepth.clear();      // Clear for next iteration
        currentDepth++;         // Increment distance
    } while (thisDepth.size() > 0);  // Continue until no more vertices to explore

    return -1;  // Unreachable (shouldn’t happen in connected CDT geometry)
}

// Calculates the shortest dual link distance between two triangles using BFS
// t1, t2: Triangles to measure distance between
// Returns number of dual hops or -1 if unreachable (Sec. 3.4)
int Observable::distanceDual(Triangle::Label t1, Triangle::Label t2) {
    if (t1 == t2) return 0;  // Same triangle: distance is 0

    std::vector<bool> done;                // Tracks visited triangles
    std::vector<Triangle::Label> thisDepth; // Current depth’s triangles
    std::vector<Triangle::Label> nextDepth; // Next depth’s triangles

    // Find maximum triangle label to size done array
    int tmax = 0;
    for (auto t : Universe::triangles) {
        if (t > tmax) tmax = t;
    }
    done.resize(tmax + 1, false);  // Initialize all as unvisited

    done.at(t1) = true;        // Mark start triangle as visited
    thisDepth.push_back(t1);   // Start BFS from t1

    int currentDepth = 0;      // Track depth (distance)
    do {
        for (auto t : thisDepth) {  // Explore neighbors at current depth
            for (auto neighbor : Universe::triangleNeighbors[t]) {
                if (neighbor == t2) return currentDepth + 1;  // Found target: return distance
                if (!done.at(neighbor)) {  // If neighbor unvisited
                    nextDepth.push_back(neighbor);  // Add to next depth
                    done.at(neighbor) = true;       // Mark as visited
                }
            }
        }
        thisDepth = nextDepth;  // Move to next depth
        nextDepth.clear();      // Clear for next iteration
        currentDepth++;         // Increment distance
    } while (thisDepth.size() > 0);  // Continue until no more triangles to explore

    return -1;  // Unreachable (shouldn’t happen in connected CDT geometry)
}
