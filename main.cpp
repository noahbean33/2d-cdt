// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#include "config.hpp"        // Handles configuration file parsing
#include "pool.hpp"          // Memory management for simplices
#include "bag.hpp"           // Random access structure for simplices
#include "vertex.hpp"        // Vertex class for triangulation nodes
#include "triangle.hpp"      // Triangle class for 2D CDT building blocks
#include "universe.hpp"      // Represents the CDT geometry and state
#include "simulation.hpp"    // Manages Monte Carlo simulation logic
#include "observable.hpp"    // Base class for measurable quantities
#include "observables/volume_profile.hpp"   // Observable: volume per time slice
#include "observables/hausdorff.hpp"        // Observable: Hausdorff dimension
#include "observables/hausdorff_dual.hpp"   // Dual lattice Hausdorff dimension (unused here)
#include "observables/ricci.hpp"            // Ricci curvature (unused here)
#include "observables/ricci_dual.hpp"       // Dual Ricci curvature (unused here)
#include "observables/riccih.hpp"           // Horizontal Ricci curvature (unused here)
#include "observables/ricciv.hpp"           // Vertical Ricci curvature (unused here)
#include <algorithm>            // For std::find and std::accumulate

int main(int argc, const char * argv[]) {
    // Variable to store config file name from command line
    std::string fname;

    // Check if a config file is provided as a command-line argument
    if (argc > 1) {
        fname = std::string(argv[1]);    // Assign first argument as file name
        printf("%s\n", fname.c_str());   // Print file name for confirmation
    }

    // Create ConfigReader instance to parse the config file
    ConfigReader cfr;
    cfr.read(fname);    // Load parameters from file (e.g., config.txt)

    // Extract simulation parameters from config
    double lambda = cfr.getDouble("lambda");           // Cosmological constant (typically ln(2))
    int targetVolume = cfr.getInt("targetVolume");     // Target number of triangles
    int slices = cfr.getInt("slices");                 // Number of time slices
    std::string sphereString = cfr.getString("sphere"); // String flag for spherical topology
    if (sphereString == "true") {                      // Check if spherical topology is enabled
        Universe::sphere = true;                       // Set static flag in Universe class
        printf("sphere\n");                            // Confirm spherical mode
    }

    int seed = cfr.getInt("seed");                     // RNG seed for reproducibility
    std::string fID = cfr.getString("fileID");         // File identifier for output
    int measurements = cfr.getInt("measurements");     // Number of observable measurements
    std::string impGeomString = cfr.getString("importGeom"); // String flag for geometry import
    bool impGeom = false;                              // Boolean to control geometry import
    if (impGeomString == "true") impGeom = true;       // Enable import if "true"

    // Attempt to import existing geometry if specified
    if (impGeom) {
        // Generate expected geometry filename based on parameters
        std::string geomFn = Universe::getGeometryFilename(targetVolume, slices, seed);
        if (geomFn != "") {                            // If a matching file exists
            Universe::importGeometry(geomFn);          // Load geometry from geom/ directory
        } else {                                       // If no file is found
            printf("No suitable geometry file found. Creating new Universe...\n");
        }
    }

    // If no geometry was imported, create a new Universe
    if (Universe::imported == false) {
        Universe::create(slices);                      // Initialize CDT with given slices
    }

    // Register observables for simulation
    VolumeProfile vp(fID);                             // Volume profile observable with fileID
    Simulation::addObservable(vp);                     // Add to simulation for measurement

    Hausdorff haus(fID);                               // Hausdorff dimension observable
    Simulation::addObservable(haus);                   // Add to simulation

    // Print seed for logging/debugging
    printf("seed: %d\n", seed);

    // Launch the Monte Carlo simulation
    Simulation::start(measurements, lambda, targetVolume, seed);
    // Parameters: number of measurements, cosmological constant, target volume, seed

    // Signal completion
    printf("end\n");
    return 0;                                          // Exit successfully
}
