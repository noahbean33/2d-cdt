// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include "universe.hpp"     // Header for Universe class, managing CDT geometry

// Initialize static members of Universe class
int Universe::nSlices = 0;  // Number of time slices, set by create() or importGeometry()
std::vector<int> Universe::sliceSizes;  // Number of vertices per time slice
bool Universe::sphere = false;  // Flag for spherical topology, set by config
bool Universe::imported = false;  // Flag indicating if geometry was imported
std::default_random_engine Universe::rng(0);  // RNG for geometry ops (TODO: seed properly)
Bag<Triangle, Triangle::pool_size> Universe::trianglesAll(rng);  // Bag of all triangles (add move candidates)
Bag<Vertex, Vertex::pool_size> Universe::verticesFour(rng);  // Bag of order-4 vertices (delete move candidates)
Bag<Triangle, Triangle::pool_size> Universe::trianglesFlip(rng);  // Bag of flippable triangles (flip move candidates)

// Lists of all simplices and their neighbors
std::vector<Vertex::Label> Universe::vertices;  // All vertices in the triangulation
std::vector<Link::Label> Universe::links;  // All links (edges)
std::vector<Triangle::Label> Universe::triangles;  // All triangles
std::vector<std::vector<Vertex::Label>> Universe::vertexNeighbors;  // Vertex neighbor lists
std::vector<std::vector<Triangle::Label>> Universe::triangleNeighbors;  // Triangle neighbor lists
std::vector<std::vector<Link::Label>> Universe::vertexLinks;  // Links per vertex
std::vector<std::vector<Link::Label>> Universe::triangleLinks;  // Links per triangle

// Creates a new CDT geometry with specified time slices
void Universe::create(int nSlices_) {
    nSlices = nSlices_;  // Set number of time slices
    initialize();  // Build initial triangulation
}

// Initializes the CDT geometry with a minimal toroidal strip
void Universe::initialize() {
    int w = 3;  // Initial width of each spatial slice (adjustable for thermalization)
    int t = nSlices;  // Total time slices

    std::vector<Vertex::Label> initialVertices(w * t);  // Array for initial vertices

    // Create vertices, assigning time coordinates
    for (int i = 0; i < w * t; i++) {
        auto v = Vertex::create();  // Allocate a new vertex
        v->time = i / w;  // Assign time based on row (i/w)
        initialVertices[i] = v;  // Store in array
    }

    // Initialize slice sizes (all set to w initially)
    for (int i = 0; i < t; i++) {
        sliceSizes.push_back(w);
    }

    // Create triangles (2 per vertex pair across time slices)
    std::vector<Triangle::Label> initialTriangles(2 * w * t);
    for (int i = 0; i < t; i++) {
        for (int j = 0; j < w; j++) {
            // Upward triangle (2,1)-type
            auto tl = Triangle::create();
            tl->setVertices(
                initialVertices[i * w + j],  // Left vertex at time i
                initialVertices[i * w + (j + 1) % w],  // Right vertex at time i
                initialVertices[((i + 1) % t) * w + j]);  // Center vertex at time i+1
            initialTriangles[2 * (i * w + j)] = tl;

            // Downward triangle (1,2)-type
            auto tr = Triangle::create();
            tr->setVertices(
                initialVertices[((i + 1) % t) * w + j],  // Left vertex at time i+1
                initialVertices[((i + 1) % t) * w + (j + 1) % w],  // Right vertex at time i+1
                initialVertices[i * w + (j + 1) % w]);  // Center vertex at time i
            initialTriangles[2 * (i * w + j) + 1] = tr;

            // Add triangles to tracking bags
            trianglesAll.add(tl);
            trianglesAll.add(tr);
            trianglesFlip.add(tl);  // Initially flippable due to opposite types
            trianglesFlip.add(tr);
        }
    }

    // Set triangle connectivity (left, right, center neighbors)
    int row = 0, column = 0;
    for (int i = 0; i < t; ++i) {
        for (int j = 0; j < w; ++j) {
            row = 2 * i * w;
            column = 2 * j;
            // Upward triangle connectivity (periodic in space and time)
            initialTriangles[row + column]->setTriangles(
                initialTriangles[row + (column - 1 + 2 * w) % (2 * w)],  // Left neighbor
                initialTriangles[row + column + 1],  // Right neighbor
                initialTriangles[(row + column - 2 * w + 1 + 2 * t * w) % (2 * t * w)]);  // Center neighbor

            // Downward triangle connectivity
            initialTriangles[row + column + 1]->setTriangles(
                initialTriangles[row + column],  // Left neighbor
                initialTriangles[row + (column + 2) % (2 * w)],  // Right neighbor
                initialTriangles[(row + column + 2 * w) % (2 * t * w)]);  // Center neighbor
        }
    }
}

// Inserts a vertex into a triangle, splitting it into four ((2,4)-move, Sec. 2.2.1)
void Universe::insertVertex(Triangle::Label t) {
    Triangle::Label tc = t->getTriangleCenter();  // Center neighbor (opposite time orientation)
    Vertex::Label vr = t->getVertexRight();  // Right vertex of original triangle
    int time = t->time;  // Time coordinate of insertion

    Vertex::Label v = Vertex::create();  // Create new vertex
    v->time = time;  // Set its time
    verticesFour.add(v);  // Add to order-4 vertex bag (new vertex starts with 4 triangles)
    sliceSizes[time] += 1;  // Increment slice size

    // Update existing triangles
    t->setVertexRight(v);  // Replace right vertex with new vertex
    tc->setVertexRight(v);

    // Create two new triangles
    Triangle::Label t1 = Triangle::create();  // Right neighbor of t
    Triangle::Label t2 = Triangle::create();  // Right neighbor of tc
    trianglesAll.add(t1);
    trianglesAll.add(t2);

    t1->setVertices(v, vr, t->getVertexCenter());  // New upward triangle
    t2->setVertices(v, vr, tc->getVertexCenter());  // New downward triangle

    // Set connectivity for new triangles
    t1->setTriangles(t, t->getTriangleRight(), t2);
    t2->setTriangles(tc, tc->getTriangleRight(), t1);

    // Update flippable triangle bag
    if (t1->type != t1->getTriangleRight()->type) {  // Check if t1 is flippable
        trianglesFlip.remove(t);  // Original t no longer flippable
        trianglesFlip.add(t1);  // New t1 is flippable
    }
    if (t2->type != t2->getTriangleRight()->type) {  // Check if t2 is flippable
        trianglesFlip.remove(tc);
        trianglesFlip.add(t2);
    }
}

// Removes a vertex of order 4, collapsing four triangles into two ((4,2)-move, Sec. 2.2.1)
void Universe::removeVertex(Vertex::Label v) {
    Triangle::Label tl = v->getTriangleLeft();  // Left upward triangle
    Triangle::Label tr = v->getTriangleRight();  // Right upward triangle
    Triangle::Label tlc = tl->getTriangleCenter();  // Left downward triangle
    Triangle::Label trc = tr->getTriangleCenter();  // Right downward triangle

    Triangle::Label trn = tr->getTriangleRight();  // Neighbor to right of tr
    Triangle::Label trcn = trc->getTriangleRight();  // Neighbor to right of trc

    // Update connectivity: merge triangles by removing tr and trc
    tl->setTriangleRight(trn);
    tlc->setTriangleRight(trcn);

    tl->setVertexRight(tr->getVertexRight());  // Extend tl to cover tr’s space
    tlc->setVertexRight(tr->getVertexRight());  // Extend tlc to cover trc’s space

    tr->getVertexRight()->setTriangleLeft(tl);  // Update vertex pointer

    sliceSizes[v->time] -= 1;  // Decrement slice size

    // Update bags and destroy removed triangles
    trianglesAll.remove(tr);
    trianglesAll.remove(trc);
    if (trianglesFlip.contains(tr)) {  // Update flippable status
        trianglesFlip.remove(tr);
        trianglesFlip.add(tl);  // tl may now be flippable
    }
    if (trianglesFlip.contains(trc)) {
        trianglesFlip.remove(trc);
        trianglesFlip.add(tlc);
    }

    Triangle::destroy(tr);  // Free memory for removed triangles
    Triangle::destroy(trc);

    verticesFour.remove(v);  // Remove vertex from order-4 bag
    Vertex::destroy(v);  // Free memory for vertex
}

// Flips a timelike link between a triangle and its right neighbor ((2,2)-move, Sec. 2.2.2)
void Universe::flipLink(Triangle::Label t) {
    auto tr = t->getTriangleRight();  // Right neighbor
    auto tc = t->getTriangleCenter();  // Center neighbor of t
    auto trc = tr->getTriangleCenter();  // Center neighbor of tr

    // Update vertex pointers based on triangle orientation
    if (t->isUpwards()) {
        t->getVertexLeft()->setTriangleRight(tr);  // Adjust left vertex
        t->getVertexRight()->setTriangleLeft(tr);  // Adjust right vertex
    } else if (t->isDownwards()) {
        tr->getVertexLeft()->setTriangleRight(t);
        tr->getVertexRight()->setTriangleLeft(t);
    }

    // Swap center neighbors
    t->setTriangleCenter(trc);
    tr->setTriangleCenter(tc);

    // Get vertex labels for reassignment
    auto vl = t->getVertexLeft();
    auto vr = t->getVertexRight();
    auto vc = t->getVertexCenter();
    auto vrr = tr->getVertexRight();

    // Reassign vertices to flip the link
    t->setVertices(vc, vrr, vl);  // New t configuration
    tr->setVertices(vl, vr, vrr);  // New tr configuration

    // Update verticesFour bag based on new coordination numbers
    if (verticesFour.contains(vl)) verticesFour.remove(vl);  // vl may no longer be order-4
    if (isFourVertex(vr)) verticesFour.add(vr);  // vr may now be order-4
    if (isFourVertex(vc)) verticesFour.add(vc);  // vc may now be order-4
    if (verticesFour.contains(vrr)) verticesFour.remove(vrr);  // vrr may no longer be order-4

    // Update trianglesFlip bag based on new neighbor types
    if (trianglesFlip.contains(t->getTriangleLeft()) && (t->type == t->getTriangleLeft()->type))
        trianglesFlip.remove(t->getTriangleLeft());  // Left neighbor no longer flippable
    if (trianglesFlip.contains(tr) && (tr->type == tr->getTriangleRight()->type))
        trianglesFlip.remove(tr);  // tr no longer flippable
    if ((!trianglesFlip.contains(t->getTriangleLeft())) && (t->type != t->getTriangleLeft()->type))
        trianglesFlip.add(t->getTriangleLeft());  // Left neighbor now flippable
    if ((!trianglesFlip.contains(tr)) && (tr->type != tr->getTriangleRight()->type))
        trianglesFlip.add(tr);  // tr now flippable
}

// Checks if a vertex has exactly 4 neighboring triangles (for delete move eligibility)
bool Universe::isFourVertex(Vertex::Label v) {
    return (v->getTriangleLeft()->getTriangleRight() == v->getTriangleRight())  // Upward neighbors match
        && (v->getTriangleLeft()->getTriangleCenter()->getTriangleRight() == v->getTriangleRight()->getTriangleCenter());  // Downward neighbors match
}

// Verifies triangulation integrity (e.g., manifold conditions, bag consistency)
void Universe::check() {
    // Check triangle connectivity and bag consistency
    for (auto t : trianglesAll) {
        assert(t->getTriangleLeft() >= 0);  // Valid left neighbor
        assert(t->getTriangleRight() >= 0);  // Valid right neighbor
        assert(t->getTriangleCenter() >= 0);  // Valid center neighbor

        assert(t->getVertexLeft() >= 0);  // Valid left vertex
        assert(t->getVertexRight() >= 0);  // Valid right vertex
        assert(t->getVertexCenter() >= 0);  // Valid center vertex

        // Verify trianglesFlip consistency
        if (trianglesFlip.contains(t)) {
            assert(t->type != t->getTriangleRight()->type);  // Must differ from right neighbor
        } else {
            assert(t->type == t->getTriangleRight()->type);  // Must match right neighbor
        }
    }

    // Verify coordination numbers for upward triangles
    for (auto t : trianglesAll) {
        if (t->isDownwards()) continue;

        auto v = t->getVertexLeft();
        int nu = 1;  // Count upward neighbors
        auto tl = v->getTriangleLeft();
        while (tl->getTriangleRight() != v->getTriangleRight()) {
            tl = tl->getTriangleRight();
            nu++;
        }
        nu++;

        int nd = 1;  // Count downward neighbors
        tl = v->getTriangleLeft()->getTriangleCenter();
        while (tl->getTriangleRight() != v->getTriangleRight()->getTriangleCenter()) {
            tl = tl->getTriangleRight();
            nd++;
        }
        nd++;

        // Verify verticesFour membership
        if (nu + nd == 4) {
            assert(Universe::verticesFour.contains(v));  // Should be in bag
        } else {
            assert(!Universe::verticesFour.contains(v));  // Should not be in bag
        }
    }

    // Check order-4 vertices’ triangle connectivity
    for (auto v : verticesFour) {
        auto tl = v->getTriangleLeft();
        auto tr = v->getTriangleRight();
        assert(tl->getTriangleRight() == tr);  // Left and right must align
        assert(tr->getTriangleLeft() == tl);
    }
}

// Updates vertex neighbor lists for measurement (Sec. 3.2.1)
void Universe::updateVertexData() {
    vertices.clear();
    int max = 0;
    // Collect vertices from upward triangles
    for (auto t : trianglesAll) {
        if (t->isUpwards()) {
            auto v = t->getVertexLeft();
            vertices.push_back(v);
            if (v > max) max = v;  // Track maximum vertex label
        }
    }

    vertexNeighbors.clear();
    vertexNeighbors.resize(max + 1);  // Resize to accommodate all vertices
    for (auto v : vertices) {
        if (sphere) {  // Special handling for spherical topology boundaries
            if (v->time == 0) {  // Bottom slice
                auto tl = v->getTriangleLeft();
                Triangle::Label tn = tl;
                do {
                    vertexNeighbors.at(v).push_back(tn->getVertexLeft());
                    tn = tn->getTriangleRight();
                } while (tn->isDownwards());
                vertexNeighbors.at(v).push_back(tn->getVertexCenter());
                vertexNeighbors.at(v).push_back(tn->getVertexRight());
                continue;
            } else if (v->time == nSlices - 1) {  // Top slice
                auto tld = v->getTriangleLeft()->getTriangleCenter();
                auto tn = tld;
                do {
                    vertexNeighbors.at(v).push_back(tn->getVertexLeft());
                    tn = tn->getTriangleRight();
                } while (tn->isUpwards());
                vertexNeighbors.at(v).push_back(tn->getVertexCenter());
                vertexNeighbors.at(v).push_back(tn->getVertexRight());
                continue;
            }
        }

        // General case: traverse neighbors in both directions
        auto tl = v->getTriangleLeft();
        Triangle::Label tn = tl;
        do {
            vertexNeighbors.at(v).push_back(tn->getVertexLeft());
            tn = tn->getTriangleRight();
        } while (tn->isDownwards());
        vertexNeighbors.at(v).push_back(tn->getVertexCenter());
        vertexNeighbors.at(v).push_back(tn->getVertexRight());

        tn = tn->getTriangleCenter()->getTriangleLeft();
        while (tn->isUpwards()) {
            vertexNeighbors.at(v).push_back(tn->getVertexRight());
            tn = tn->getTriangleLeft();
        }
        vertexNeighbors.at(v).push_back(tn->getVertexCenter());
    }
}

// Updates link data (edges) for measurement
void Universe::updateLinkData() {
    // Clear existing links
    for (auto l : links) {
        Link::destroy(l);
    }
    links.clear();
    int max = 0;

    // Resize adjacency lists
    vertexLinks.clear();
    for (auto i = 0u; i < vertexNeighbors.size(); i++) {
        vertexLinks.push_back({});
    }
    triangleLinks.clear();
    for (auto i = 0u; i < triangleNeighbors.size(); i++) {
        triangleLinks.push_back({-1, -1, -1});  // Three links per triangle (left, right, center)
    }

    // Create links for all triangles
    for (auto t : trianglesAll) {
        auto ll = Link::create();  // Left timelike link
        if (t->isUpwards()) ll->setVertices(t->getVertexLeft(), t->getVertexCenter());
        else if (t->isDownwards()) ll->setVertices(t->getVertexCenter(), t->getVertexLeft());
        ll->setTriangles(t->getTriangleLeft(), t);  // Connect to left neighbor

        vertexLinks.at(t->getVertexLeft()).push_back(ll);
        vertexLinks.at(t->getVertexCenter()).push_back(ll);

        triangleLinks.at(t).at(0) = ll;  // Left link slot
        triangleLinks.at(t->getTriangleLeft()).at(1) = ll;  // Right link slot of left neighbor
        links.push_back(ll);
        if (ll > max) max = ll;

        if (t->isUpwards()) {  // Horizontal spacelike link for upward triangles
            auto lh = Link::create();
            lh->setVertices(t->getVertexLeft(), t->getVertexRight());
            lh->setTriangles(t, t->getTriangleCenter());

            vertexLinks.at(t->getVertexLeft()).push_back(lh);
            vertexLinks.at(t->getVertexRight()).push_back(lh);

            triangleLinks.at(t).at(2) = lh;  // Center link slot
            triangleLinks.at(t->getTriangleCenter()).at(2) = lh;

            links.push_back(lh);
            if (lh > max) max = lh;
        }
    }

    assert(links.size() == 3 * vertices.size());  // Verify link count (triangular lattice property)
}

// Updates triangle neighbor lists for measurement
void Universe::updateTriangleData() {
    triangles.clear();
    int max = 0;
    for (auto t : trianglesAll) {
        triangles.push_back(t);
        if (t > max) max = t;  // Track maximum triangle label
    }

    triangleNeighbors.clear();
    triangleNeighbors.resize(max + 1);  // Resize to accommodate all triangles
    for (auto t : trianglesAll) {
        if (sphere) {  // Special handling for boundary triangles in spherical topology
            if (t->isUpwards() && t->time == 0) {
                triangleNeighbors.at(t) = {t->getTriangleLeft(), t->getTriangleRight()};
                continue;
            }
            if (t->isDownwards() && t->time == nSlices - 1) {
                triangleNeighbors.at(t) = {t->getTriangleLeft(), t->getTriangleRight()};
                continue;
            }
        }

        // General case: all three neighbors
        triangleNeighbors-at(t) = {t->getTriangleLeft(), t->getTriangleRight(), t->getTriangleCenter()};
    }
}

// Exports current geometry to a file for checkpointing
void Universe::exportGeometry(std::string geometryFilename) {
    std::unordered_map<int, int> vertexMap;  // Map vertex labels to indices
    std::vector<Vertex::Label> intVMap;  // Reverse mapping for writing
    intVMap.resize(vertices.size());

    int i = 0;
    for (auto v : vertices) {
        vertexMap.insert({v, i});  // Assign index to vertex
        intVMap.at(i) = v;
        i++;
    }

    std::unordered_map<int, int> triangleMap;  // Map triangle labels to indices
    std::vector<Triangle::Label> intTMap;
    intTMap.resize(triangles.size());

    i = 0;
    for (auto t : triangles) {
        triangleMap.insert({t, i});  // Assign index to triangle
        intTMap.at(i) = t;
        i++;
    }

    std::string output;
    output += std::to_string(vertices.size()) + "\n";  // Write vertex count

    // Write vertex times
    for (int j = 0; j < intVMap.size(); j++) {
        output += std::to_string(intVMap.at(j)->time) + "\n";
    }

    output += std::to_string(vertices.size()) + "\n";  // Vertex count again (format quirk)
    output += std::to_string(triangles.size()) + "\n";  // Triangle count

    // Write triangle data (vertices and neighbors)
    for (int j = 0; j < intTMap.size(); j++) {
        Triangle::Label t = intTMap.at(j);
        Vertex::Label tVs[3] = {t->getVertexLeft(), t->getVertexRight(), t->getVertexCenter()};
        for (auto v : tVs) {
            output += std::to_string(vertexMap.at(v)) + "\n";  // Vertex indices
        }
        Triangle::Label tNeighb[3] = {t->getTriangleLeft(), t->getTriangleRight(), t->getTriangleCenter()};
        for (auto t : tNeighb) {
            output += std::to_string(triangleMap.at(t)) + "\n";  // Neighbor indices
        }
    }

    output += std::to_string(triangles.size());  // Triangle count again (format quirk)

    // Write to file
    std::ofstream file;
    file.open(geometryFilename, std::ios::out | std::ios::trunc);  // Overwrite mode
    assert(file.is_open());
    file << output << "\n";
    file.close();

    std::cout << geometryFilename << "\n";  // Log export
}

// Imports a geometry from a file, reconstructing the triangulation
void Universe::importGeometry(std::string geometryFilename) {
    std::ifstream infile(geometryFilename.c_str());
    assert(!infile.fail());  // Ensure file exists
    int line;

    int nV;
    infile >> nV;  // Read number of vertices
    std::vector<Vertex::Label> vs(nV);

    int maxTime = 0;
    // Create vertices with time coordinates
    for (int i = 0; i < nV; i++) {
        infile >> line;
        auto v = Vertex::create();
        v->time = line;
        vs.at(i) = v;
        if (v->time > maxTime) maxTime = v->time;
    }
    infile >> line;
    assert(line == nV);  // Verify vertex count

    nSlices = maxTime + 1;  // Set number of slices
    sliceSizes.resize(maxTime + 1);
    std::fill(sliceSizes.begin(), sliceSizes.end(), 0);  // Initialize slice sizes

    int nT;
    infile >> nT;  // Read number of triangles
    for (int i = 0; i < nT; i++) {
        auto t = Triangle::create();
        int tVs[3];  // Vertex indices
        for (int j = 0; j < 3; j++) {
            infile >> tVs[j];
        }
        int tNeighb[3];  // Neighbor indices
        for (int j = 0; j < 3; j++) {
            infile >> tNeighb[j];
        }
        t->setVertices(tVs[0], tVs[1], tVs[2]);  // Set vertices (to be resolved later)
        t->setTriangles(tNeighb[0], tNeighb[1], tNeighb[2]);  // Set neighbors (to be resolved)
        trianglesAll.add(t);  // Add to main bag
    }
    infile >> line;
    assert(line == nT);  // Verify triangle count

    printf("read %s\n", geometryFilename.c_str());  // Log import

    // Update slice sizes
    for (auto v : vs) sliceSizes.at(v->time)++;
    if (sphere) assert(sliceSizes.at(0) == 3);  // Verify spherical boundary

    // Populate bag data post-import
    for (auto t : trianglesAll) {
        if (t->isUpwards()) {
            auto v = t->getVertexLeft();
            if (v->getTriangleLeft() == v->getTriangleRight()->getTriangleLeft()
                && v->getTriangleLeft()->getTriangleCenter() == v->getTriangleRight()->getTriangleCenter()->getTriangleLeft()) {
                verticesFour.add(v);  // Add order-4 vertices
            }
        }
        if (t->type != t->getTriangleRight()->type) {
            trianglesFlip.add(t);  // Add flippable triangles
        }
    }

    check();  // Validate imported geometry
    imported = true;  // Mark as imported
}

// Generates a standardized filename for geometry files
std::string Universe::getGeometryFilename(int targetVolume, int slices, int seed) {
    std::string expectedFn = "geom/geometry-v" + std::to_string(targetVolume) +
                            "-t" + std::to_string(slices) +
                            "-s" + std::to_string(seed);
    if (sphere) expectedFn += "-sphere";  // Append spherical flag if set
    expectedFn += ".dat";  // File extension
    return expectedFn;
}
