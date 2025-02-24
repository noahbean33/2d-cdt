// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <stdio.h>      // For standard I/O (unused here, likely a leftover)
#include "pool.hpp"     // Base class Pool for efficient memory management
#include "vertex.hpp"   // Vertex class, used for triangle vertex connectivity

// Triangle class, inheriting from Pool for memory allocation
class Triangle : public Pool<Triangle> {
public:
    // Maximum number of triangles in the pool (twice the vertex pool size, reflecting 2 triangles per vertex pair)
    static const unsigned pool_size = 2 * Vertex::pool_size;

    // Enum defining triangle orientation: UP (2,1)-type or DOWN (1,2)-type (Sec. 2.2)
    enum Type { UP, DOWN };

    // Time slice index at the triangle's base (aligned with left/right vertices)
    int time;

    // Orientation of the triangle (UP or DOWN), set by updateType()
    Type type;

    // Returns the label of the left neighboring triangle (index in Pool<Triangle>)
    // noexcept: Guarantees no exceptions, for performance in frequent calls
    Triangle::Label getTriangleLeft() const noexcept { return tl; }

    // Returns the label of the right neighboring triangle
    Triangle::Label getTriangleRight() const noexcept { return tr; }

    // Returns the label of the center neighboring triangle (opposite time orientation)
    Triangle::Label getTriangleCenter() const noexcept { return tc; }

    // Sets the left neighboring triangle and updates its right pointer bidirectionally
    // t: Label of the triangle to set as left neighbor
    void setTriangleLeft(Triangle::Label t) {
        tl = t;        // Assign left neighbor
        t->tr = *this; // Update t’s right neighbor to this triangle
    }

    // Sets the right neighboring triangle and updates its left pointer bidirectionally
    void setTriangleRight(Triangle::Label t) {
        tr = t;        // Assign right neighbor
        t->tl = *this; // Update t’s left neighbor to this triangle
    }

    // Sets the center neighboring triangle and updates its center pointer bidirectionally
    void setTriangleCenter(Triangle::Label t) {
        tc = t;        // Assign center neighbor
        t->tc = *this; // Update t’s center neighbor to this triangle
    }

    // Sets all three neighboring triangles at once, ensuring bidirectional consistency
    // tl_: Left neighbor, tr_: Right neighbor, tc_: Center neighbor
    void setTriangles(Triangle::Label tl_, Triangle::Label tr_, Triangle::Label tc_) {
        tl = tl_;      // Assign left neighbor
        tr = tr_;      // Assign right neighbor
        tc = tc_;      // Assign center neighbor
        tl_->tr = *this; // Update left neighbor’s right pointer
        tr_->tl = *this; // Update right neighbor’s left pointer
        tc_->tc = *this; // Update center neighbor’s center pointer
    }

    // Returns the label of the left vertex (index in Pool<Vertex>)
    Vertex::Label getVertexLeft() const noexcept { return vl; }

    // Returns the label of the right vertex
    Vertex::Label getVertexRight() const noexcept { return vr; }

    // Returns the label of the center vertex (apex)
    Vertex::Label getVertexCenter() const noexcept { return vc; }

    // Sets the left vertex and updates its connectivity if upward
    // v: Label of the vertex to set as left
    void setVertexLeft(Vertex::Label v) {
        vl = v;           // Assign left vertex
        time = v->time;   // Set triangle’s time to match left vertex
        if (type == UP) { // If upward, update vertex’s right triangle pointer
            v->setTriangleRight(*this);
        }
    }

    // Sets the right vertex and updates its connectivity if upward
    void setVertexRight(Vertex::Label v) {
        vr = v;           // Assign right vertex
        if (type == UP) { // If upward, update vertex’s left triangle pointer
            v->setTriangleLeft(*this);
        }
    }

    // Sets all three vertices at once, updating type and connectivity
    // vl_: Left vertex, vr_: Right vertex, vc_: Center vertex
    void setVertices(Vertex::Label vl_, Vertex::Label vr_, Vertex::Label vc_) {
        vl = vl_;         // Assign left vertex
        vr = vr_;         // Assign right vertex
        vc = vc_;         // Assign center vertex
        time = vl_->time; // Set time based on left vertex
        updateType();     // Determine orientation (UP/DOWN)
        if (type == UP) { // If upward, update vertex pointers
            vl_->setTriangleRight(*this);
            vr_->setTriangleLeft(*this);
        }
    }

    // Sets the center vertex (apex) without updating connectivity
    void setVertexCenter(Vertex::Label v) { vc = v; }

    // Checks if the triangle is upward (2,1)-type
    bool isUpwards() {
        return type == UP;
    }

    // Checks if the triangle is downward (1,2)-type
    bool isDownwards() {
        return type == DOWN;
    }

private:
    // Labels (indices) of neighboring triangles in Pool<Triangle>
    Pool<Triangle>::Label tl, tr, tc;  // Left, right, center neighbors

    // Labels (indices) of vertices in Pool<Vertex>
    Pool<Vertex>::Label vl, vr, vc;    // Left, right, center vertices

    // Updates the triangle’s type (UP/DOWN) based on vertex times
    // Handles periodic boundary conditions for toroidal topology
    void updateType() {
        if (vl->time < vc->time) {  // Left vertex earlier than center: upward
            type = UP;
        } else {                    // Center earlier than left: downward
            type = DOWN;
        }
        // Special cases for periodic boundaries
        if (vc->time == 0 && vl->time > 1) {  // Center at t=0, left later: upward
            type = UP;
        }
        if (vl->time == 0 && vc->time > 1) {  // Left at t=0, center later: downward
            type = DOWN;
        }
    }
};
