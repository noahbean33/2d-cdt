// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include "link.hpp"     // Header for Link class, defining structure and interface
#include "vertex.hpp"   // Vertex class, used for endpoint connectivity
#include "triangle.hpp" // Triangle class, used for bordering triangles

// Returns the label of the "final" vertex (endpoint) of the link
// Simply retrieves the stored vf member (index in Pool<Vertex>)
// In CDT, "final" often implies the vertex at a later time for timelike links
Vertex::Label Link::getVertexFinal() {
    return vf;
}

// Returns the label of the "initial" vertex (starting point) of the link
// Retrieves the stored vi member (index in Pool<Vertex>)
// Typically the vertex at an earlier or same time slice
Vertex::Label Link::getVertexInitial() {
    return vi;
}

// Returns the label of the "plus" triangle bordering the link
// Retrieves the stored tp member (index in Pool<Triangle>)
// Represents one of the two triangles sharing this edge (orientation-based)
Triangle::Label Link::getTrianglePlus() {
    return tp;
}

// Returns the label of the "minus" triangle bordering the link
// Retrieves the stored tm member (index in Pool<Triangle>)
// Represents the other triangle sharing this edge (opposite orientation)
Triangle::Label Link::getTriangleMinus() {
    return tm;
}

// Sets the initial and final vertices of the link
// vi_: Initial vertex label, vf_: Final vertex label
// Updates vi and vf members to define the link’s endpoints
void Link::setVertices(Vertex::Label vi_, Vertex::Label vf_) {
    vi = vi_;   // Assign initial vertex
    vf = vf_;   // Assign final vertex
}

// Sets the plus and minus triangles bordering the link
// tp_: Plus triangle label, tm_: Minus triangle label
// Updates tp and tm members to associate the link with adjacent triangles
void Link::setTriangles(Triangle::Label tp_, Triangle::Label tm_) {
    tp = tp_;   // Assign plus triangle
    tm = tm_;   // Assign minus triangle
}

// Checks if the link is timelike (connects vertices at different time slices)
// Returns true if vi and vf have different time values, indicating a time step
bool Link::isTimelike() {
    return vf->time != vi->time;
}

// Checks if the link is spacelike (connects vertices within the same time slice)
// Returns true if vi and vf have the same time value, indicating a spatial connection
bool Link::isSpacelike() {
    return vf->time == vi->time;
}
