// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <random>       // Provides random number generation (e.g., std::default_random_engine)
#include <vector>       // Used for storing pointers to Observable objects
#include "universe.hpp" // Defines Universe class, representing the CDT geometry
#include "observable.hpp" // Base class for observables measured during simulation

class Simulation {
public:
    // Static cosmological constant, set by main() from config file (typically ln(2) for 2D CDT)
    static double lambda;
    
    // Static seed for random number generator, set by main() for reproducibility
    static int seed;

    // Initiates the Monte Carlo simulation with specified parameters
    // sweeps: number of measurement sweeps to perform
    // lambda_: cosmological constant for action computation
    // targetVolume_: desired number of triangles
    // seed_: RNG seed (defaults to 0 if not provided)
    static void start(int sweeps, double lambda_, int targetVolume_, int seed_ = 0);

    // Adds an observable to the simulation for measurement
    // o: reference to an Observable object (e.g., VolumeProfile, Hausdorff)
    // Stores pointer in observables vector
    static void addObservable(Observable& o) {
        observables.push_back(&o);
    }

    // Flag indicating if topology pinching is allowed (not used in current 2D setup)
    static bool pinch;

    // Tracks frequency of move attempts: [0] for add/delete, [1] for flip
    // Used to monitor simulation dynamics
    static std::array<int, 2> moveFreqs;

    // Attempts a single Monte Carlo move (add, delete, or flip)
    // Returns number of successful moves (typically 0 or 1)
    static int attemptMove();

private:
    // Random number generator for Monte Carlo moves, seeded by seed
    static std::default_random_engine rng;

    // Target number of triangles, set by start() to guide volume-fixing
    static int targetVolume;

    // Strength of volume-fixing term (S_fix = epsilon * (N - targetVolume)^2)
    // Controls fluctuations around targetVolume
    static double epsilon;

    // Flag indicating if simulation is in measurement phase (vs. thermalization)
    static bool measuring;

    // Vector of pointers to observables registered for measurement
    // Populated by addObservable()
    static std::vector<Observable*> observables;

    // Performs one sweep: a batch of move attempts (size depends on targetVolume)
    // Core of Monte Carlo sampling
    static void sweep();

    // Attempts an "add" move ((2,4)-move): adds two triangles
    // Returns true if accepted, false if rejected (per Metropolis algorithm)
    static bool moveAdd();

    // Attempts a "delete" move ((4,2)-move): removes two triangles
    // Returns true if accepted, false if rejected
    static bool moveDelete();

    // Attempts a "flip" move ((2,2)-move): flips a timelike edge
    // Returns true if accepted, false if rejected
    static bool moveFlip();

    // Prepares geometry for measurement by reconstructing connectivity
    // Called before observable computation (e.g., neighbor lists for BFS)
    static void prepare();

    // Tuning function to adjust lambda to pseudocritical value
    // Commented out as unused in 2D CDT (lambda fixed at ln(2))
    // static void tune();

    // Grows the triangulation to targetVolume during initialization
    static void grow();

    // Thermalizes the system: runs sweeps to reach equilibrium
    // Ensures initial geometry bias is removed before measurements
    static void thermalize();
};
