// Copyright 2020 Joren Brunekreef and Andrzej Görlich
#pragma once    // Ensures this header is included only once during compilation

#include <string>           // For std::string (e.g., identifier, name)
#include "../observable.hpp" // Base class Observable, providing measurement framework
#include "../universe.hpp"   // Provides access to Universe’s geometry data (e.g., sliceSizes)

// VolumeProfile class, inheriting from Observable to measure volume per time slice
class VolumeProfile : public Observable {
public:
    // Constructor: initializes the observable with a file identifier
    // id: String identifier for output files (e.g., "collab-16000-1")
    VolumeProfile(std::string id) : Observable(id) {
        name = "volume_profile";  // Set observable name for file naming and identification
    }

    // Implements the pure virtual process() method from Observable
    // Computes the volume (number of vertices) for each time slice in the CDT geometry
    // Stores result in output string for writing (Sec. 3.4)
    void process();
};
