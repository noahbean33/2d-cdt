// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <string>       // For std::string (e.g., identifier, output)
#include <vector>       // For storing vertex/triangle labels in sphere methods
#include "universe.hpp" // Provides access to Universe’s geometry data (e.g., vertices, triangles)

// Observable base class for measuring properties of CDT geometries
class Observable {
public:
    // Name of the observable (e.g., "VolumeProfile"), set by derived classes
    std::string name;

    // Constructor: initializes the observable with a file identifier
    // identifier_: String used to name output files (e.g., "collab-16000-1")
    Observable(std::string identifier_) {
        identifier = identifier_;  // Store identifier for file output
    }

    // Performs a single measurement: processes data and writes results
    // Calls virtual process() (implemented by derived classes) and write()
    void measure() {
        process();  // Compute the observable’s value
        write();    // Write result to file
    }

    // Clears stored data (e.g., output) to reset for new measurements
    void clear();

private:
    // Identifier for output files, set by constructor
    std::string identifier;

protected:
    // Static random number generator shared across all Observable instances
    // Used for random vertex/triangle selection
    static std::default_random_engine rng;

    // Pure virtual function: derived classes must implement specific measurement logic
    // Processes Universe data to compute the observable’s value (e.g., volume profile)
    virtual void process() = 0;

    // Writes the computed observable data (from output) to a file
    // Uses identifier, data_dir, and extension for file naming
    void write();

    // Toolbox: Utility methods for derived classes

    // Computes a metric sphere of given radius around a vertex
    // origin: Starting vertex, radius: Distance in link hops
    // Returns vector of vertices within radius (uses BFS, Sec. 3.4)
    static std::vector<Vertex::Label> sphere(Vertex::Label origin, int radius);

    // Computes a dual metric sphere of given radius around a triangle
    // origin: Starting triangle, radius: Distance in dual link hops
    // Returns vector of triangles within radius (uses BFS, Sec. 3.4)
    static std::vector<Triangle::Label> sphereDual(Triangle::Label origin, int radius);

    // Calculates the shortest link distance between two vertices
    // v1, v2: Vertices to measure distance between
    // Returns number of hops (uses BFS, Sec. 3.4)
    static int distance(Vertex::Label v1, Vertex::Label v2);

    // Calculates the shortest dual link distance between two triangles
    // t1, t2: Triangles to measure distance between
    // Returns number of dual hops (uses BFS, Sec. 3.4)
    static int distanceDual(Triangle::Label t1, Triangle::Label t2);

    // Selects a random vertex from Universe::vertices
    // Returns its label using uniform distribution
    static Vertex::Label randomVertex() {
        std::uniform_int_distribution<> rv(0, Universe::vertices.size() - 1);
        return Universe::vertices.at(rv(rng));
    }

    // Selects a random triangle from Universe::triangles
    // Returns its label using uniform distribution
    static Triangle::Label randomTriangle() {
        std::uniform_int_distribution<> rt(0, Universe::triangles.size() - 1);
        return Universe::triangles.at(rt(rng));
    }

    // Directory for output files (default: "out/")
    std::string data_dir = "out/";

    // File extension for output (default: ".dat")
    std::string extension = ".dat";

    // String buffer storing the observable’s computed data
    // Populated by process(), written by write()
    std::string output;
};
