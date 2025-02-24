// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include "simulation.hpp"
#include <vector>           // Used for storing observable pointers and volume data

// Initialize static members of Simulation class
std::default_random_engine Simulation::rng(0);  // Random number generator, initially seeded with 0 (TODO: proper seeding)
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
    rng.seed(seed);                  // Seed the random number generator
    // tune();                       // Tuning is disabled in 2D CDT (lambda fixed at ln(2))

    // If no geometry was imported, initialize and prepare it
    if (!Universe::imported) {
        grow();                      // Grow triangulation to targetVolume
        thermalize();                // Thermalize to remove initial bias
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
            if (moveAdd()) return 1;    // Success: add move executed
        } else {                // 50% chance for delete
            if (moveDelete()) return 2; // Success: delete move executed
        }
    } else if (cumFreqs[0] <= move) {   // Flip move
        if (moveFlip()) return 3;       // Success: flip move executed
    }

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

    // Adjust volume to exactly match targetVolume
    do {
        attemptMove();
    } while (Triangle::size() != targetVolume);

    prepare();    // Reconstruct geometry connectivity for measurement
    // Measure all registered observables
    for (auto o : observables) {
        o->measure();
    }
}

// Attempts an "add" move ((2,4)-move): adds two triangles
bool Simulation::moveAdd() {
    double n0 = Vertex::size();         // Current number of vertices
    // double n2 = Triangle::size();    // Current number of triangles (commented out alternative)
    double n0_four = Universe::verticesFour.size(); // Number of vertices of order four

    // Acceptance ratio using bookkeeping method (Sec. 2.2.1, Eq. 19)
    double ar = n0 / (n0_four + 1.0) * exp(-2 * lambda);
    if (targetVolume > 0) {     // Apply volume-fixing term if target is set
        double expesp = exp(2 * epsilon);
        // Boost/reduce acceptance based on current vs. target volume
        ar *= Triangle::size() < targetVolume ? expesp : 1 / expesp;
    }

    Triangle::Label t = Universe::trianglesAll.pick(); // Randomly select a triangle

    // Reject move if spherical topology and triangle is at time 0 (boundary condition)
    if (Universe::sphere) {
        if (t->time == 0) return false;
    }

    // Metropolis acceptance: compare random number to acceptance ratio
    if (ar < 1.0) {
        std::uniform_real_distribution<> uniform(0.0, 1.0);
        double r = uniform(rng);
        if (r > ar) return false;    // Reject move
    }

    Universe::insertVertex(t);    // Execute add move: insert vertex, add two triangles
    return true;                  // Move accepted
}

// Attempts a "delete" move ((4,2)-move): removes two triangles
bool Simulation::moveDelete() {
    // Reject if no vertices of order four are available
    if (Universe::verticesFour.size() == 0) return false;

    double n0 = Vertex::size();         // Current number of vertices
    // double n2 = Triangle::size();    // Current number of triangles (commented out)
    double n0_four = Universe::verticesFour.size(); // Number of vertices of order four

    // Acceptance ratio using bookkeeping method (Sec. 2.2.1, Eq. 20)
    double ar = n0_four / (n0 - 1.0) * exp(2 * lambda);
    if (targetVolume > 0) {     // Apply volume-fixing term
        double expesp = exp(2 * epsilon);
        // Boost/reduce acceptance based on current vs. target volume
        ar *= Triangle::size() < targetVolume ? 1 / expesp : expesp;
    }

    // Metropolis acceptance check
    if (ar < 1.0) {
        std::uniform_real_distribution<> uniform(0.0, 1.0);
        double r = uniform(rng);
        if (r > ar) return false;    // Reject move
    }

    Vertex::Label v = Universe::verticesFour.pick(); // Pick a vertex of order four
    // Reject if slice size would drop below 4 (maintains manifold condition)
    if (Universe::sliceSizes[v->time] < 4) return false;

    Universe::removeVertex(v);    // Execute delete move: remove vertex and two triangles
    return true;                  // Move accepted
}

// Attempts a "flip" move ((2,2)-move): flips a timelike edge
bool Simulation::moveFlip() {
    // Reject if no flippable triangles exist
    if (Universe::trianglesFlip.size() == 0) return false;

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
        if (r > ar) return false;    // Reject move
    }

    Universe::flipLink(t);    // Execute flip move: swap timelike edge
    return true;              // Move accepted
}

// Prepares geometry for measurement by updating connectivity data
void Simulation::prepare() {
    Universe::updateVertexData();    // Refresh vertex neighbor lists
    Universe::updateTriangleData();  // Refresh triangle neighbor lists
    Universe::updateLinkData();      // Refresh link data (edges)
}

// Disabled tuning function (unused in 2D CDT as lambda is fixed)
#if 0
void Simulation::tune() {
    printf("start tune..\n");
    fflush(stdout);
    std::vector<int> volumes;    // Store volume samples during tuning
    epsilon = 0.02;              // Initial volume-fixing strength

    bool done = false;
    int tuneSteps = 50;          // Maximum tuning iterations
    for (int k = 0; k < tuneSteps && !done; k++) {
        // Run sweeps and collect volume data
        for (int i = 0; i < targetVolume; i++) {
            for (int j = 0; j < 100; j++) attemptMove();
            volumes.push_back(Triangle::size());
        }

        // Compute average volume
        double avg = 0.0;
        for (auto v : volumes) avg += static_cast<double>(v);
        avg /= volumes.size();

        // Compute standard deviation
        double sd = 0.0;
        for (auto v : volumes) sd += (static_cast<double>(v) - avg) * (static_cast<double>(v) - avg);
        sd /= volumes.size();

        // Adjust epsilon and lambda based on volume deviation
        if ((targetVolume - avg)*(targetVolume - avg) < 2*sd) {
            epsilon *= 0.7;    // Tighten volume control
            if (epsilon < 0.02) {
                epsilon = 0.02;
                lambda -= 0.003 * (targetVolume - avg) / sqrt(sd); // Adjust lambda
            }
        } else if ((targetVolume - avg)*(targetVolume - avg) > 8*sd) {
            epsilon *= 1.2;    // Loosen volume control
            if (epsilon > 5.0) epsilon = 5.0;
        } else if ((targetVolume - avg)*(targetVolume - avg) < 0.04*targetVolume*targetVolume) {
            lambda += 0.6*(avg - targetVolume)/abs((avg-targetVolume)) * epsilon; // Fine-tune lambda
        }
        volumes.clear();
        // Stop if close to target and epsilon stabilized
        if (k >= tuneSteps && abs(avg-targetVolume) < 0.1*targetVolume && epsilon < 0.021) done = true;

        printf("step %d - epsilon: %f, lambda: %f, avg: %f, sd: %f\n", k, epsilon, lambda, avg, sd);
    }
}
#endif

// Grows the triangulation to reach targetVolume
void Simulation::grow() {
    int growSteps = 0;
    printf("growing");
    do {
        // Perform targetVolume move attempts per step
        for (int i = 0; i < targetVolume; i++) attemptMove();
        printf(".");
        fflush(stdout);
        growSteps++;
    } while (Triangle::size() < targetVolume);    // Continue until target is reached
    printf("\n");
    printf("grown in %d sweeps\n", growSteps);
}

// Thermalizes the system to remove initial geometry bias
void Simulation::thermalize() {
    int thermSteps = 0;
    printf("thermalizing");
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
    } while (maxUp > coordBound || maxDown > coordBound);    // Continue until coordination stabilizes
    printf("\n");
    printf("thermalized in %d sweeps\n", thermSteps);
}
