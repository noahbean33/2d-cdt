// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include "simulation.hpp"
#include <vector>           // Used for storing observable pointers and volume data
#include <algorithm>        // For std::find and std::accumulate
#include <iostream>         // Added for std::cout debugging output

// Initialize static members of Simulation class
std::default_random_engine Simulation::rng(0);  // Random number generator, initially seeded with 0
int Simulation::targetVolume = 0;               // Target number of triangles, set by start()
double Simulation::lambda = 0;                  // Cosmological constant, set by start() (typically ln(2) for 2D CDT)
int Simulation::seed = 0;                       // RNG seed for reproducibility, set by start()
double Simulation::epsilon = 0.02;              // Volume-fixing term strength (S_fix = epsilon * (N - targetVolume)^2)
std::vector<Observable*> Simulation::observables; // Vector of registered observables (e.g., VolumeProfile)
std::array<int, 2> Simulation::moveFreqs = {1, 1}; // Frequency of move types: [0] add/delete, [1] flip

// Starts the Monte Carlo simulation with specified parameters
void Simulation::start(int measurements, double lambda_, int targetVolume_, int seed_) {
    targetVolume = targetVolume_;    // Set target number of triangles
    lambda = lambda_;                // Set cosmological constant

    // Clear previous measurement data from all registered observables
    for (auto o : observables) {
        o->clear();
    }

    seed = seed_;                    // Set RNG seed
    rng.seed(seed + 0);              // Seed Simulation's RNG with base_seed + 0
    Universe::seedRNG(seed, 1);      // Seed Universe's RNG with base_seed + 1

    // If no geometry was imported, initialize and prepare it
    if (!Universe::imported) {
        std::cout << "Starting simulation with target volume: " << targetVolume << std::endl;
        grow();                      // Grow triangulation to targetVolume
        thermalize();                // Thermalize to remove initial bias
        Simulation::prepare();       // Update geometry data before exporting
        // Export initial geometry to geom/ directory
        Universe::exportGeometry(Universe::getGeometryFilename(targetVolume, Universe::nSlices, seed));
    }

    // Run measurement phase: perform specified number of sweeps
    for (int i = 0; i < measurements; i++) {
        sweep();                     // Execute one sweep (batch of moves)
        printf("m %d\n", i);         // Print measurement progress
        // Export geometry every 10 measurements for checkpointing
        if (i % 10 == 0) Universe::exportGeometry(Universe::getGeometryFilename(targetVolume, Universe::nSlices, seed));
        fflush(stdout);              // Flush output buffer for real-time logging
    }
    std::cout << "Simulation completed with " << measurements << " measurements." << std::endl;
}

// Attempts a single Monte Carlo move (add, delete, or flip)
int Simulation::attemptMove() {
    std::array<int, 2> cumFreqs = {0, 0}; // Cumulative frequencies for move selection
    int freqTotal = 0;                    // Total frequency sum
    int prevCumFreq = 0;                  // Previous cumulative frequency

    // Compute cumulative frequencies based on moveFreqs
    for (auto i = 0u; i < moveFreqs.size(); i++) {
        freqTotal += moveFreqs[i];
        cumFreqs[i] = prevCumFreq + moveFreqs[i];
        prevCumFreq = cumFreqs[i];
    }

    std::uniform_int_distribution<> moveGen(0, freqTotal-1); // Random move selector
    std::uniform_int_distribution<> binGen(0, 1);            // Binary choice (add vs. delete)

    int move = moveGen(rng);    // Pick a move based on frequency distribution

    // Execute move based on cumulative frequency ranges
    if (move < cumFreqs[0]) {   // Add or delete move
        if (binGen(rng) == 0) { // 50% chance for add
            if (moveAdd()) {
                // std::cout << "AttemptMove: Add move succeeded." << std::endl; // Uncomment for verbose logging
                return 1;    // Success: add move executed
            }
        } else {                // 50% chance for delete
            if (moveDelete()) {
                // std::cout << "AttemptMove: Delete move succeeded." << std::endl; // Uncomment for verbose logging
                return 2; // Success: delete move executed
            }
        }
    } else if (cumFreqs[0] <= move) {   // Flip move
        if (moveFlip()) {
            // std::cout << "AttemptMove: Flip move succeeded." << std::endl; // Uncomment for verbose logging
            return 3;       // Success: flip move executed
        }
    }

    // std::cout << "AttemptMove: No move executed." << std::endl; // Uncomment for verbose logging
    return 0;   // No move executed (rejected or invalid)
}

// Performs one sweep: a batch of move attempts to sample geometry
void Simulation::sweep() {
    std::uniform_int_distribution<> uniform_int(0, 3); // Unused in current implementation

    std::array<int, 4> moves = {0, 0, 0, 0};    // Track move successes: [0] none, [1] add, [2] delete, [3] flip
    // Perform 100 * targetVolume move attempts (defines sweep size)
    for (int i = 0; i < 100 * targetVolume; i++) {
        moves[attemptMove()]++;    // Attempt move and increment corresponding counter
    }
    std::cout << "Sweep completed - Moves: [Rejected: " << moves[0] << ", Add: " << moves[1] 
              << ", Delete: " << moves[2] << ", Flip: " << moves[3] << "]" << std::endl;

    // Adjust volume to exactly match targetVolume
    int adjustAttempts = 0;
    do {
        attemptMove();
        adjustAttempts++;
        if (adjustAttempts % 1000 == 0) {
            std::cout << "Volume adjustment in progress: " << Universe::trianglesAll.size() 
                      << " triangles after " << adjustAttempts << " attempts" << std::endl;
        }
    } while (Universe::trianglesAll.size() != targetVolume); // Use correct size metric
    std::cout << "Volume adjusted to " << targetVolume << " triangles in " << adjustAttempts << " attempts" << std::endl;

    prepare();    // Reconstruct geometry connectivity for measurement
    // Measure all registered observables
    for (auto o : observables) {
        o->measure();
    }
}

// Attempts an "add" move ((2,4)-move): adds two triangles
bool Simulation::moveAdd() {
    double n0 = Vertex::size();         // Current number of vertices
    double n0_four = Universe::verticesFour.size(); // Number of vertices of order four

    // Acceptance ratio using bookkeeping method (Sec. 2.2.1, Eq. 19)
    double ar = n0 / (n0_four + 1.0) * exp(-2 * lambda);
    if (targetVolume > 0) {     // Apply volume-fixing term if target is set
        double expesp = exp(2 * epsilon);
        // Boost/reduce acceptance based on current vs. target volume
        ar *= Universe::trianglesAll.size() < targetVolume ? expesp : 1 / expesp; // Use correct size
    }

    Triangle::Label t = Universe::trianglesAll.pick(); // Randomly select a triangle
    if (Universe::trianglesAll.size() == 0) {
        std::cout << "moveAdd: Error - trianglesAll bag is empty!" << std::endl;
        return false;
    }

    // Reject move if spherical topology and triangle is at time 0 (boundary condition)
    if (Universe::sphere) {
        if (t->time == 0) {
            std::cout << "moveAdd: Rejected - triangle at time 0 (boundary condition)" << std::endl;
            return false;
        }
    }

    // Metropolis acceptance: compare random number to acceptance ratio
    if (ar < 1.0) {
        std::uniform_real_distribution<> uniform(0.0, 1.0);
        double r = uniform(rng);
        if (r > ar) {
            std::cout << "moveAdd: Rejected - random " << r << " > acceptance ratio " << ar << std::endl;
            return false;    // Reject move
        }
    }

    int oldSize = Universe::trianglesAll.size();
    Universe::insertVertex(t);    // Execute add move: insert vertex, add two triangles
    std::cout << "moveAdd: Accepted - Added vertex to triangle " << t 
              << ", triangles increased from " << oldSize << " to " << Universe::trianglesAll.size() << std::endl;
    return true;                  // Move accepted
}

// Attempts a "delete" move ((4,2)-move): removes two triangles
bool Simulation::moveDelete() {
    // Reject if no vertices of order four are available
    if (Universe::verticesFour.size() == 0) {
        std::cout << "moveDelete: Rejected - no vertices of order four available" << std::endl;
        return false;
    }

    double n0 = Vertex::size();         // Current number of vertices
    double n0_four = Universe::verticesFour.size(); // Number of vertices of order four

    // Acceptance ratio using bookkeeping method (Sec. 2.2.1, Eq. 20)
    double ar = n0_four / (n0 - 1.0) * exp(2 * lambda);
    if (targetVolume > 0) {     // Apply volume-fixing term
        double expesp = exp(2 * epsilon);
        // Boost/reduce acceptance based on current vs. target volume
        ar *= Universe::trianglesAll.size() < targetVolume ? 1 / expesp : expesp; // Use correct size
    }

    // Metropolis acceptance check
    if (ar < 1.0) {
        std::uniform_real_distribution<> uniform(0.0, 1.0);
        double r = uniform(rng);
        if (r > ar) {
            std::cout << "moveDelete: Rejected - random " << r << " > acceptance ratio " << ar << std::endl;
            return false;    // Reject move
        }
    }

    Vertex::Label v = Universe::verticesFour.pick(); // Pick a vertex of order four
    // Reject if slice size would drop below 4 (maintains manifold condition)
    if (Universe::sliceSizes[v->time] < 4) {
        std::cout << "moveDelete: Rejected - slice size at time " << v->time << " would drop below 4" << std::endl;
        return false;
    }

    int oldSize = Universe::trianglesAll.size();
    Universe::removeVertex(v);    // Execute delete move: remove vertex and two triangles
    std::cout << "moveDelete: Accepted - Removed vertex " << v 
              << ", triangles decreased from " << oldSize << " to " << Universe::trianglesAll.size() << std::endl;
    return true;                  // Move accepted
}

// Attempts a "flip" move ((2,2)-move): flips a timelike edge
bool Simulation::moveFlip() {
    // Reject if no flippable triangles exist
    if (Universe::trianglesFlip.size() == 0) {
        std::cout << "moveFlip: Rejected - no flippable triangles available" << std::endl;
        return false;
    }

    auto t = Universe::trianglesFlip.pick();    // Pick a flippable triangle

    int wa = Universe::trianglesFlip.size();    // Number of flippable triangles before move
    int wb = wa;                                // Number after move (adjusted below)
    // Adjust wb based on type changes of neighboring triangles (Sec. 2.2.2)
    if (t->type == t->getTriangleLeft()->type) {
        wb++;    // Left neighbor becomes flippable
    } else {
        wb--;    // Left neighbor becomes unflippable
    }
    if (t->getTriangleRight()->type == t->getTriangleRight()->getTriangleRight()->type) {
        wb++;    // Right neighbor becomes flippable
    } else {
        wb--;    // Right neighbor becomes unflippable
    }

    double ar = 1.0 * wa / wb;    // Acceptance ratio based on flippable count change (Eq. 22)

    // Metropolis acceptance check
    if (ar < 1.0) {
        std::uniform_real_distribution<> uniform(0.0, 1.0);
        double r = uniform(rng);
        if (r > ar) {
            std::cout << "moveFlip: Rejected - random " << r << " > acceptance ratio " << ar << std::endl;
            return false;    // Reject move
        }
    }

    Universe::flipLink(t);    // Execute flip move: swap timelike edge
    std::cout << "moveFlip: Accepted - Flipped link for triangle " << t << std::endl;
    return true;              // Move accepted
}

// Prepares geometry for measurement by updating connectivity data
void Simulation::prepare() {
    std::cout << "Preparing geometry data..." << std::endl;
    Universe::updateVertexData();    // Refresh vertex neighbor lists
    Universe::updateTriangleData();  // Refresh triangle neighbor lists
    Universe::updateLinkData();      // Refresh link data (edges)
    std::cout << "Geometry data updated." << std::endl;
}

// Grows the triangulation to reach targetVolume
void Simulation::grow() {
    int growSteps = 0;
    printf("growing");
    std::cout << "Growth phase started. Initial triangles: " << Universe::trianglesAll.size() << std::endl;
    do {
        // Perform 10 * targetVolume move attempts per step for faster growth
        for (int i = 0; i < 10 * targetVolume; i++) attemptMove();
        printf(".");
        fflush(stdout);
        growSteps++;
        std::cout << "Grow sweep " << growSteps << ": " << Universe::trianglesAll.size() << " triangles" << std::endl;
    } while (Universe::trianglesAll.size() < targetVolume); // Use correct size metric
    printf("\n");
    printf("grown in %d sweeps\n", growSteps);
    std::cout << "Growth phase completed with " << Universe::trianglesAll.size() << " triangles in " 
              << growSteps << " sweeps" << std::endl;
}

// Thermalizes the system to remove initial geometry bias
void Simulation::thermalize() {
    int thermSteps = 0;
    printf("thermalizing");
    std::cout << "Thermalization phase started." << std::endl;
    fflush(stdout);
    // Coordination number bound to ensure equilibrium (logarithmic scaling)
    double coordBound = log(2 * targetVolume) / static_cast<double>(log(2));
    int maxUp, maxDown;    // Maximum upward/downward coordination numbers
    do {
        // Perform 100 * targetVolume move attempts per step
        for (int i = 0; i < 100 * targetVolume; i++) attemptMove();
        printf(".");
        fflush(stdout);

        prepare();    // Update connectivity data
        maxUp = 0;
        maxDown = 0;
        // Check coordination numbers for all vertices
        for (auto v : Universe::vertices) {
            int nup = 0, ndown = 0;
            for (auto vn : Universe::vertexNeighbors.at(v)) {
                // Count upward connections (including periodic boundary)
                if (vn->time > v->time || (v->time == Universe::nSlices-1 && vn->time == 0)) nup++;
                // Count downward connections
                if (vn->time < v->time || (v->time == 0 && vn->time == Universe::nSlices-1)) ndown++;
            }
            if (nup > maxUp) maxUp = nup;
            if (ndown > maxDown) maxDown = ndown;
        }
        thermSteps++;
        std::cout << "Thermalization sweep " << thermSteps << ": maxUp = " << maxUp 
                  << ", maxDown = " << maxDown << ", coordBound = " << coordBound << std::endl;
    } while (maxUp > coordBound || maxDown > coordBound);    // Continue until coordination stabilizes
    printf("\n");
    printf("thermalized in %d sweeps\n", thermSteps);
    std::cout << "Thermalization completed in " << thermSteps << " sweeps" << std::endl;
}