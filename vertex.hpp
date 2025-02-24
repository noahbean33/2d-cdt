// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include "pool.hpp"     // Base class Pool for memory management of simplices

// Forward declaration of Triangle class (defined elsewhere)
class Triangle;

// Vertex class, inheriting from Pool for efficient memory allocation
class Vertex : public Pool<Vertex> {
public:
    // Maximum number of vertices that can be stored in the pool (10 million)
    static const unsigned pool_size = 10000000;

    // Time slice index where this vertex resides (0 to nSlices-1)
    int time;

    // Returns the label (index) of the left neighboring triangle
    // This is one of the upward triangles containing the vertex
    Pool<Triangle>::Label getTriangleLeft();

    // Returns the label (index) of the right neighboring triangle
    // This is the other upward triangle containing the vertex
    Pool<Triangle>::Label getTriangleRight();

    // Sets the left neighboring triangle
    // t: Label of the triangle to set as left neighbor
    void setTriangleLeft(Pool<Triangle>::Label t);

    // Sets the right neighboring triangle
    // t: Label of the triangle to set as right neighbor
    void setTriangleRight(Pool<Triangle>::Label t);

private:
    // Label (index) of the left upward triangle in the Pool<Triangle>
    // Stored as a private member to maintain vertex-triangle connectivity
    Pool<Triangle>::Label tl;

    // Label (index) of the right upward triangle in the Pool<Triangle>
    // Stored as a private member to maintain vertex-triangle connectivity
    Pool<Triangle>::Label tr;
};
