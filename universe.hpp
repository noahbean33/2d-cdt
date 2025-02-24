// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <vector>           // Used for storing lists of simplices and neighbors
#include <random>           // Provides random number generation for geometry operations
#include <iostream>         // For console output (e.g., debugging)
#include <fstream>          // For file I/O (geometry export/import)
#include <unordered_map>    // Used for efficient neighbor lookups
#include "vertex.hpp"       // Vertex class: nodes in the triangulation
#include "link.hpp"         // Link class: edges connecting vertices
#include "triangle.hpp"     // Triangle class: 2D simplices (building blocks of CDT)
#include "pool.hpp"         // Pool structure for O(1) simplex management
#include "bag.hpp"          // Bag structure for random access to simplices

class Universe {
public:
    // Number of time slices in the CDT geometry (set by create() or config)
    static int nSlices;

    // Size (number of vertices) of each time slice, indexed by time
    static std::vector<int> sliceSizes;

    // Flag to enforce spherical topology (optional, set by config)
    static bool sphere;

    // Flag indicating if geometry was imported from a file (set by importGeometry())
    static bool imported;

    // Bag of all triangles, candidates for the add move ((2,4)-move, Sec. 2.2.1)
    static Bag<Triangle, Triangle::pool_size> trianglesAll;

    // Bag of vertices with coordination number 4, candidates for the delete move ((4,2)-move, Sec. 2.2.1)
    static Bag<Vertex, Vertex::pool_size> verticesFour;

    // Bag of triangles with a right neighbor of opposite type, candidates for the flip move ((2,2)-move, Sec. 2.2.2)
    static Bag<Triangle, Triangle::pool_size> trianglesFlip;

    // Initializes static data structures (e.g., bags, vectors) before simulation
    static void initialize();

    // Creates a new CDT geometry with specified number of time slices
    // n_slices: number of discrete time steps in the triangulation
    static void create(int n_slices);

    // Monte Carlo moves (Sec. 2.2)

    // Inserts a vertex into a triangle, splitting it into four triangles ((2,4)-move)
    static void insertVertex(Triangle::Label t);

    // Removes a vertex of order four, collapsing four triangles into two ((4,2)-move)
    static void removeVertex(Vertex::Label v);

    // Enum to specify flip direction (left or right neighbor) for flipLink()
    enum flipSide { LEFT, RIGHT };

    // Flips a timelike link adjacent to a vertex, specified by direction
    static void flipLink(Vertex::Label v, flipSide side);

    // Flips a timelike link shared by a triangle and its right neighbor ((2,2)-move)
    static void flipLink(Triangle::Label t);

    // Bag consistency functions

    // Updates a vertex’s coordination numbers (up/down) and adjusts verticesFour bag
    // v: vertex to update, up/down: number of upward/downward connections
    static void updateVertexCoord(Vertex::Label v, int up, int down);

    // Checks if a vertex has coordination number 4 (eligible for delete move)
    static bool isFourVertex(Vertex::Label v);

    // Verifies integrity of the triangulation (e.g., manifold conditions, Sec. 1.3)
    static void check();

    // Updates connectivity data for measurement (Sec. 3.2.1)

    // Refreshes vertex neighbor lists (vertexNeighbors)
    static void updateVertexData();

    // Refreshes link data (vertexLinks, triangleLinks)
    static void updateLinkData();

    // Refreshes triangle neighbor lists (triangleNeighbors)
    static void updateTriangleData();

    // Exports current geometry to a file for checkpointing or reuse
    static void exportGeometry(std::string geometryFilename);

    // Imports a saved geometry from a file, bypassing creation
    static void importGeometry(std::string geometryFilename);

    // Generates a standardized filename for geometry based on parameters
    // targetVolume: target number of triangles, slices: time slices, seed: RNG seed
    static std::string getGeometryFilename(int targetVolume, int slices, int seed);

    // Lists of all simplices in the triangulation (populated during simulation)
    static std::vector<Vertex::Label> vertices;       // All vertices
    static std::vector<Link::Label> links;           // All links (edges)
    static std::vector<Triangle::Label> triangles;   // All triangles

    // Neighbor adjacency lists (reconstructed by update*Data() for measurements)
    static std::vector<std::vector<Vertex::Label>> vertexNeighbors;    // Neighbors of each vertex
    static std::vector<std::vector<Triangle::Label>> triangleNeighbors; // Neighbors of each triangle

    // Link adjacency lists (used for connectivity and measurements)
    static std::vector<std::vector<Link::Label>> vertexLinks;     // Links connected to each vertex
    static std::vector<std::vector<Link::Label>> triangleLinks;   // Links bordering each triangle

private:
    // Private default constructor to prevent instantiation (Universe is static-only)
    Universe() {}

    // Random number generator for geometry operations (e.g., initial creation)
    static std::default_random_engine rng;
};
