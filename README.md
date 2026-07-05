# Pool Allocator
This is a C++ implementation of a Pool Allocator for use in my personal projects.

## Description
This project was originally created when I wanted to create a nice way to loop over a bunch of objects without having a ton of cache misses!

The allocator works by initially grabbing enough memory for all the possible objects in the pool.
It then uses a free-list embedded within the unused storage to know what is available.
This requires every chunk of the pool to be at least the size of a pointer, so the pool is best used when the size of 
the object being stored is at least 8 bytes. This also means every chunk is aligned to, at minimum, 8 bytes.

The allocator can also be moved, which changes ownership of the block of memory. As of now there is no split between the
allocator and the actual storage object itself, so you will not be able to pass the allocator into STL objects that want an allocator.
This is because as a result of the storage and allocator being the same the copy constructor has no implementation, as it doesn't
make sense to copy the whole block of memory. If we did copy the whole block of memory any blocks allocated before the copy would be filled with
no pointer out. Also, the free-list being embedded in the list would cause copies to have their free-list elements pointing to the original pool.
Moved PoolAllocator objects simply have their pool and free list moved. Attempting to construct a new element from a moved-from list
will just return a nullptr as the pool has no capacity.

## Installation
When using the allocator the only thing you need is the `PoolAllocator.hpp` file.

To run the benchmarks, simply build with CMake. `cmake -B build && cmake --build build`

## Usage
```c++
#include "PoolAllocator.hpp"

struct Vector {
    float x;
    float y;
};

int main() {
    // Create a pool of 10,000 vectors
    auto pool {PoolAllocator<Vector> (10000)};

    // Returns nullptr if the pool is full and the object cannot be constructed
    Vector* vec = pool.construct();
    
    // ... Do work ...
    
    // Clean up
    pool.destroy(vec);
    vec = nullptr;
}
```

Note that the allocator falling out of scope will release its hold on all the memory, so when the pool is destroyed
anything using the pool's allocated memory will have undefined behavior.

## Benchmark
The `main.cpp` contains a benchmark for the allocator! Running these benchmarks should be as simple as what was described in installation.
They simply compare the pool allocator's performance versus raw new and delete calls on the heap.

Here are the results from the benchmark when I ran them!

CPU: AMD Ryzen 7 7700X 4.5 GHz

Compiler: MSVC 19.38.33145

Build: Release

Flags: --benchmark_repetitions=10 --benchmark_report_aggregates_only=true

```
-------------------------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------
BM_PoolParticleChurn_mean            202719 ns       195648 ns           10 items_per_second=5.11439M/s
BM_PoolParticleChurn_median          200160 ns       195648 ns           10 items_per_second=5.11122M/s
BM_PoolParticleChurn_stddev            9733 ns         5115 ns           10 items_per_second=134.817k/s
BM_PoolParticleChurn_cv                4.80 %          2.61 %            10 items_per_second=2.64%
BM_NewParticleChurn_mean             255975 ns       251674 ns           10 items_per_second=3.97511M/s
BM_NewParticleChurn_median           256396 ns       251116 ns           10 items_per_second=3.98222M/s
BM_NewParticleChurn_stddev             4350 ns         5549 ns           10 items_per_second=86.7771k/s
BM_NewParticleChurn_cv                 1.70 %          2.20 %            10 items_per_second=2.18%
BM_PoolParticleThroughput_mean       267564 ns       262690 ns           10 items_per_second=380.993M/s
BM_PoolParticleThroughput_median     267864 ns       263876 ns           10 items_per_second=379.014M/s
BM_PoolParticleThroughput_stddev       5271 ns         7931 ns           10 items_per_second=11.6094M/s
BM_PoolParticleThroughput_cv           1.97 %          3.02 %            10 items_per_second=3.05%
BM_NewParticleThroughput_mean       3589301 ns      3530844 ns           10 items_per_second=28.5316M/s
BM_NewParticleThroughput_median     3480534 ns      3449675 ns           10 items_per_second=28.9882M/s
BM_NewParticleThroughput_stddev      289266 ns       340898 ns           10 items_per_second=2.42286M/s
BM_NewParticleThroughput_cv            8.06 %          9.65 %            10 items_per_second=8.49%
```

The churn test simulates an environment of about 75,000 particles, with 1000 being created every frame and a 
variable number being destroyed every frame. In this case the pool allocator had a 28.6% higher throughput, with about
a 21% lower frame time.

The throughput test simply creates and destroys 100,000 particles every frame. In this case the pool allocator has about a
13x higher throughput than just raw new and delete calls. This however is not the best benchmark of actual performance in the real world
as there is not much reason to construct and deconstruct this many objects without doing any work.

Another note is that in the throughput test the new and delete variance is 8-10% while the pool allocator is 2-3%. This shows the consistency of
the pool allocator while handling large amounts of data compared to just new and delete. 
Within the churn tests the variance was much smaller, with 1-2% for new and delete, and the pool allocator at 2-5%.