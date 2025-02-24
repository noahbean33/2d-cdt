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
This fork is being upgraded over 12 months to enhance performance and physics capabilities using HPC (OpenMP, MPI, GPU) and machine learning (ML):

### Timeline
- **Months 1-2 (Feb-Mar 2025)**: Profile with `gprof`, collect ML training data (e.g., move rates, observables).
- **Months 3-4 (Apr-May 2025)**: Add OpenMP for parallel sweeps, integrate RL (e.g., DQN) for move optimization.
- **Months 5-6 (Jun-Jul 2025)**: Optimize `Pool/Bag` memory, predict observables (e.g., volume profiles) with ML.
- **Months 7-8 (Aug-Sep 2025)**: Implement MPI for distributed runs, scale ML training across nodes.
- **Months 9-10 (Oct-Nov 2025)**: Port moves to GPU (CUDA), extend to 4D CDT with ML support.
- **Months 11-12 (Dec 2025-Jan 2026)**: Hybrid OpenMP/MPI/GPU runs, analyze 4D physics (e.g., phase transitions).

### Goals
- **Performance**: Achieve 10-100x speedup via HPC/GPU, scale to \(N_2 = 10^7\) and beyond.
- **ML**: Optimize moves (reduce autocorrelation), predict observables, detect phases.
- **Physics**: Enable 4D CDT simulations for Planck-scale insights.
- **Repo**: Updated fork at `https://github.com/noahbean33/2d-cdt` with code, docs, and results.

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
