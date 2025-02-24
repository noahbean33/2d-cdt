// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#ifndef LINK_HPP_    // Include guard start (prevents multiple inclusions)
#define LINK_HPP_

#include "pool.hpp"     // Base class Pool for efficient memory management

// Forward declarations of Vertex and Triangle classes (defined elsewhere)
class Vertex;
class Triangle;

// Link class, inheriting from Pool for memory allocation
class Link : public Pool<Link> {
public:
    // Maximum number of links in the pool (10 million, matching Vertex pool_size)
    static const unsigned pool_size = 10000000;

    // Returns the label of the "final" vertex (endpoint) of the link
    // In CDT, direction may reflect time orientation (e.g., later time for timelike links)
    Pool<Vertex>::Label getVertexFinal();

    // Returns the label of the "initial" vertex (starting point) of the link
    // Typically the vertex at an earlier or same time slice
    Pool<Vertex>::Label getVertexInitial();

    // Returns the label of the "plus" triangle bordering the link
    // One of the two triangles sharing this edge (orientation-based naming)
    Pool<Triangle>::Label getTrianglePlus();

    // Returns the label of the "minus" triangle bordering the link
    // The other triangle sharing this edge (opposite orientation)
    Pool<Triangle>::Label getTriangleMinus();

    // Sets the initial and final vertices of the link
    // vi: Initial vertex label, vf: Final vertex label
    void setVertices(Pool<Vertex>::Label vi, Pool<Vertex>::Label vf);

    // Sets the plus and minus triangles bordering the link
    // tp: Plus triangle label, tm: Minus triangle label
    void setTriangles(Pool<Triangle>::Label tp, Pool<Triangle>::Label tm);

    // Checks if the link is timelike (connects vertices at different time slices)
    // Based on time difference between vi and vf
    bool isTimelike();

    // Checks if the link is spacelike (connects vertices within the same time slice)
    // Based on equal time of vi and vf
    bool isSpacelike();

private:
    // Labels (indices) of the initial and final vertices in Pool<Vertex>
    Pool<Vertex>::Label vi, vf;  // Vertices defining the link’s endpoints

    // Labels (indices) of the plus and minus triangles in Pool<Triangle>
    // Represent the two triangles bordering the link
    Pool<Triangle>::Label tp, tm;  // Triangles adjacent to the link
};

#endif  // LINK_HPP_    // Include guard end
