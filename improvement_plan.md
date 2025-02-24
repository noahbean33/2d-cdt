# HPC Opportunities to Improve the CDT Simulation Codebase

This document outlines specific High-Performance Computing (HPC) opportunities to enhance the performance and scalability of the 2D Causal Dynamical Triangulations (CDT) simulation codebase from *Simulating CDT Quantum Gravity* by Brunekreef et al. (2023). Opportunities are categorized by OpenMP (multi-core parallelism), MPI (distributed parallelism), GPU (massive parallelism), and general HPC optimizations, tied to specific code sections and aligned with a 12-month optimization timeline (Feb 2025-Jan 2026).

---

## OpenMP Opportunities (Phase 2: Months 3-4, Apr-May 2025)

OpenMP enables parallelism within a single node, ideal for tasks like Monte Carlo sweeps and observable measurements.

### 1. Parallelize Monte Carlo Sweeps
- **Location**: `simulation.cpp`, lines 97-105 (`for (int i = 0; i < 100 * targetVolume; i++)`)
- **Description**: The loop performs independent move attempts per sweep.
- **Improvement**: Use `#pragma omp parallel for` to distribute iterations across threads, with thread-local `Universe` copies.
  
```cpp
#pragma omp parallel for
for (int i = 0; i < 100 * targetVolume; i++) {
    moves[attemptMove()]++;  // Thread-local moves array
}
```
- **Challenges**: `Static Simulation::rng` needs thread-local RNGs; `Universe::trianglesAll` updates require privatization.
- **Benefit**: Near-linear speedup (e.g., 8x on 8 cores) for sweeps (Sec. 3.3).

### 2. Parallelize Observable Measurements
- **Location**: `simulation.cpp`, lines 44-51 (`for (int i = 0; i < measurements; i++)`)
- **Description**: Each measurement sweep is independent, calling `Observable::measure()`.
- **Improvement**: Parallelize with OpenMP for separate sweeps or observable subsets.
  
```cpp
#pragma omp parallel for
for (int i = 0; i < measurements; i++) {
    sweep();  // Thread-local Universe
}
```
- **Challenges**: Shared observables and Universe need thread-safe access; `Observable::rng` requires thread-local instances.
- **Benefit**: Scales with core count (e.g., 4-16x) for ensemble averaging (Sec. 2).

### 3. Parallel BFS in Sphere/Distance Calculations
- **Location**: `observable.cpp`, lines 33-63 (sphere), 93-126 (distance)
- **Description**: Serial BFS loops over `Universe::vertexNeighbors`.
- **Improvement**: Parallelize outer loops or use a level-synchronous BFS algorithm with OpenMP.
- **Challenges**: `done` vector updates need atomic operations; load imbalance from varying neighbors.
- **Benefit**: 2-4x speedup for large radii in Ricci* and Hausdorff observables (Sec. 3.4).

---

## MPI Opportunities (Phase 4: Months 7-8, Aug-Sep 2025)

MPI enables distributed parallelism across nodes, suitable for large systems or ensemble runs.

### 4. Distribute Monte Carlo Ensembles
- **Location**: `simulation.cpp`, lines 26-51 (`start()` function)
- **Description**: Independent simulations with different seeds can run across nodes.
- **Improvement**: Each rank runs `start()` with a unique seed, aggregating via `MPI_Reduce`.
  
```cpp
MPI_Init(&argc, &argv);
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
Simulation::start(measurements, lambda, targetVolume, seed + rank);
```
- **Challenges**: Static members (`rng`, `observables`) need per-rank instances; I/O needs rank-specific files.
- **Benefit**: Linear scaling (e.g., 32x on 32 nodes) for ensemble averages (Sec. 2).

---

## GPU Opportunities (Phase 5: Months 9-10, Oct-Nov 2025)

GPU parallelism leverages thousands of cores for compute-intensive tasks.

### 7. GPU-Accelerated Monte Carlo Moves
- **Location**: `simulation.cpp`, lines 65-95 (`attemptMove`)
- **Description**: Move proposals are independent across iterations.
- **Improvement**: Port to CUDA kernel, batching move attempts.
  
```cuda
__global__ void proposeMoves(Tetra* tetras, int* accept, int n, float lambda) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < n) accept[idx] = metropolisAccept(tetras[idx], lambda);
}
```
- **Challenges**: `Universe` updates need GPU-friendly structures; RNG requires `cuRAND`.
- **Benefit**: 10-100x speedup for sweeps (Sec. 2), critical for large \(N_2\).

### 8. GPU-Accelerated BFS for Observables
- **Location**: `observable.cpp`, lines 33-63 (sphere), 93-126 (distance)
- **Description**: BFS dominates Ricci* and Hausdorff runtime.
- **Improvement**: Implement CUDA BFS kernel with pre-allocated adjacency lists.
  
```cuda
__global__ void bfsKernel(int* neighbors, bool* done, int* depth, int n, int radius) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < n && !done[idx]) depth[idx] = currentDepth;
}
```
- **Challenges**: Dynamic `std::vector` needs static arrays; load imbalance from varying neighbors.
- **Benefit**: 10-50x speedup for large radii (Sec. 3.4).

---

## General HPC Optimizations (Across Phases, Especially 1, 3, 6)

### 10. Cache Optimization
- **Location**: `universe.cpp` (e.g., `updateVertexData`, lines 218-267), `observable.cpp` (e.g., `sphere`, lines 33-63)
- **Description**: `std::vector` resizing and scattered access hurt cache efficiency.
- **Improvement**: Pre-allocate with `reserve()` and use contiguous arrays.
  
```cpp
vertexNeighbors.reserve(vmax + 1);  // Pre-size for known max
```
- **Challenges**: Estimating max sizes dynamically.
- **Benefit**: 20-50% speedup for large systems (Phase 3).

### 12. Algorithmic Tuning
- **Location**: `simulation.cpp`, lines 97-114 (`sweep`)
- **Description**: Fixed `100 * targetVolume` moves may over-sample.
- **Improvement**: Adjust sweep size dynamically based on acceptance rates.
- **Challenges**: Balancing thermalization and accuracy.
- **Benefit**: 10-30% runtime reduction (Phase 1).

---

## Conclusion
The CDT codebase offers significant HPC opportunities in its Monte Carlo core (`Simulation`), geometry management (`Universe`), and measurement framework (`Observable`). OpenMP provides quick multi-core gains (4-16x), MPI scales to large systems and ensembles (10-50x), GPU accelerates compute-intensive tasks (10-100x), and general optimizations enhance efficiency (20-50%). These align with the 12-month timeline: profiling (Phase 1), OpenMP (Phase 2), memory/I-O (Phase 3), MPI (Phase 4), GPU (Phase 5), and hybrid tuning (Phase 6), supporting the paper’s scalability outlook (Sec. 4).

For further details, see the codebase comments or contact `https://github.com/noahbean33`.
