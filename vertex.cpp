// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include "vertex.hpp"   // Header for Vertex class, defining structure and interface
#include "triangle.hpp" // Header for Triangle class, used for connectivity labels

// Returns the label of the left neighboring triangle
// Simply retrieves the stored tl member (index in Pool<Triangle>)
// Used to access one of the upward triangles containing this vertex
Triangle::Label Vertex::getTriangleLeft() { return tl; }

// Returns the label of the right neighboring triangle
// Retrieves the stored tr member (index in Pool<Triangle>)
// Used to access the other upward triangle containing this vertex
Triangle::Label Vertex::getTriangleRight() { return tr; }

// Sets the left neighboring triangle
// t: Label (index) of the triangle to assign as the left neighbor
// Updates tl to maintain vertex-triangle connectivity
void Vertex::setTriangleLeft(Triangle::Label t) { tl = t; }

// Sets the right neighboring triangle
// t: Label (index) of the triangle to assign as the right neighbor
// Updates tr to maintain vertex-triangle connectivity
void Vertex::setTriangleRight(Triangle::Label t) { tr = t; }
