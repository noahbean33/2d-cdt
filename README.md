# Monte Carlo Simulation of Two-Dimensional Causal Dynamical Triangulations

This codebase provides a framework for sampling the partition sum of two-dimensional Causal Dynamical Triangulations (CDT), a model of 2D lattice quantum gravity. It supports custom observables for computing quantum expectation values, with simulation parameters set via a config file. This fork enhances the original code with HPC and machine learning upgrades.

## Usage
### Build with GNU Make:
```bash
make
```
### Run the example simulation in `example`:
```bash
./run.sh
```

### Config file template (required parameters):
```
lambda          0.693147
targetVolume    16000
slices          100
seed            1
fileID          collab-16000-1
measurements    100
sphere          false
importGeom      true
```
- **lambda**: Cosmological constant (typically ln(2) = 0.693147).
- **targetVolume**: Target number of triangles (even numbers only).
- **slices**: Number of time slices.
- **seed**: RNG seed (fixed output for same seed).
- **fileID**: Output file identifier.
- **measurements**: Number of observable measurements.
- **sphere**: Enforce spherical topology (see below).
- **importGeom**: Import existing geometry from `geom/`.

## Observables
Standard observables (e.g., volume profile, Hausdorff dimension) are in `observables/`. Add them in `main.cpp`. Custom observables can use `Universe` (access to `Vertex`, `Link`, `Triangle`) and `Observable` (metric spheres, distances).

## Optimization Plan (2025)

### Timeline


## Citation
If using or extending this code, cite the original work:
```bibtex
@misc{brunekreef2023simulating,
  title = {Simulating {{CDT}} quantum gravity},
  author = {Brunekreef, Joren and G{"o}rlich, Andrzej and Loll, Renate},n  year = {2023},
  month = oct,
  number = {arXiv:2310.16744},
  eprint = {2310.16744},
  primaryclass = {gr-qc, physics:hep-lat, physics:hep-th},
  publisher = {{arXiv}},
  doi = {10.48550/arXiv.2310.16744},
  urldate = {2023-11-13},
  archiveprefix = {arxiv},
  keywords = {General Relativity and Quantum Cosmology, High Energy Physics - Lattice, High Energy Physics - Theory}
}
```
Please also acknowledge this fork: `https://github.com/noahbean33/2d-cdt`.
