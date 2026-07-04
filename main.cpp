/*
 * This file exists to benchmark PoolAllocator.hpp!
 * If you're not benchmarking then you don't need to do anything with this!
 * If you are benchmarking just run it with CMake!
 */

#include <benchmark/benchmark.h>
#include <execution>
#include <iostream>
#include <random>
#include <stdexcept>

#include "PoolAllocator.hpp"

// In the benchmark
constexpr uint32_t CAPACITY = 100000;

// Generate a random seed to use for every run that needs it
// shohei ohtani 2025 playoff stats
// 68 AB, 13 runs, 18 hr, 14 rbi, 4.43 ERA
// what a player
constexpr uint64_t RANDOM_SEED = 68131814443;

struct Vector3 {
    float x;
    float y;
    float z;

    Vector3 &operator+=(const Vector3 &other) {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;

        return *this;
    }
};

struct Particle {
    Vector3 position;
    Vector3 velocity;
    uint32_t lifetime;
};

static void PoolParticleFrame(PoolAllocator<Particle> &pool, std::vector<Particle *> &particles, std::mt19937_64 &rng,
                              std::uniform_int_distribution<uint32_t> &lifetimeDistribution) {
    // every frame construct 1000 particles!
    for (std::size_t i = 0; i < 1000; i++) {
        Particle *p = pool.construct();

        if (p == nullptr) {
            throw std::runtime_error("Pool system out of available objects, increase capacity!");
        }

        p->position = {0, 0, 0};
        p->velocity = {1, 1, 1};
        p->lifetime = lifetimeDistribution(rng);

        benchmark::DoNotOptimize(p);
        particles.push_back(p);
    }
    // ensure vector state is correct after push_backs
    benchmark::ClobberMemory();

    size_t index = 0;
    while (index < particles.size()) {
        Particle *particle = particles[index];

        particle->lifetime--;

        // Particle lifetime ended
        if (particle->lifetime == 0) {
            // Swap our particle with the last particle and then pop the back
            particles[index] = particles.back();
            particles.pop_back();

            // Finally destroy our saved particle
            pool.destroy(particle);
        } else {
            ++index;

            particle->position += particle->velocity;
        }
    }
}

static void NewParticleFrame(std::vector<Particle *> &particles, std::mt19937_64 &rng,
                             std::uniform_int_distribution<uint32_t> &lifetimeDistribution) {
    // every frame construct 1000 particles!
    for (std::size_t i = 0; i < 1000; i++) {
        auto *p = new Particle();

        p->position = {0, 0, 0};
        p->velocity = {1, 1, 1};
        p->lifetime = lifetimeDistribution(rng);

        benchmark::DoNotOptimize(p);
        particles.push_back(p);
    }
    // ensure vector state is correct after push_backs
    benchmark::ClobberMemory();

    size_t index = 0;
    while (index < particles.size()) {
        Particle *particle = particles[index];

        particle->lifetime--;

        // Particle lifetime ended
        if (particle->lifetime == 0) {
            // Swap our particle with the last particle and then pop the back
            particles[index] = particles.back();
            particles.pop_back();

            // Finally destroy our saved particle
            delete particle;
        } else {
            ++index;

            particle->position += particle->velocity;
        }
    }
}

static void BM_PoolParticleChurn(benchmark::State &state) {
    // Create a pool outside the benchmark, it's a one time cost so not really important for use in a game that
    // runs over a very large timespan
    auto pool{PoolAllocator<Particle>(CAPACITY)};

    std::vector<Particle *> particles{};
    particles.reserve(CAPACITY);

    std::mt19937_64 rng{RANDOM_SEED};
    std::uniform_int_distribution<uint32_t> lifetimeDistribution(30, 120);

    // 120 frame warmup
    for (size_t i = 0; i < 120; i++) {
        PoolParticleFrame(pool, particles, rng, lifetimeDistribution);
    }

    for (auto _: state) {
        PoolParticleFrame(pool, particles, rng, lifetimeDistribution);
    }

    state.SetItemsProcessed(state.iterations() * 1000);

    // make sure vector is empty
    for (const auto particle: particles) {
        pool.destroy(particle);
    }
    particles.clear();
}

static void BM_NewParticleChurn(benchmark::State &state) {
    // Create a pool outside the benchmark, it's a one time cost so not really important for use in a game that
    // runs over a very large timespan
    auto pool{PoolAllocator<Particle>(CAPACITY)};

    std::vector<Particle *> particles{};
    particles.reserve(CAPACITY);

    std::mt19937_64 rng{RANDOM_SEED};
    std::uniform_int_distribution<uint32_t> lifetimeDistribution(30, 120);

    // 120 frame warmup
    for (size_t i = 0; i < 120; i++) {
        NewParticleFrame(particles, rng, lifetimeDistribution);
    }

    for (auto _: state) {
        NewParticleFrame(particles, rng, lifetimeDistribution);
    }

    state.SetItemsProcessed(state.iterations() * 1000);

    // make sure vector is empty
    for (const auto particle: particles) {
        pool.destroy(particle);
    }
    particles.clear();
}

static void BM_PoolParticleThroughput(benchmark::State &state) {
    // Create a pool outside the benchmark, it's a one time cost so not really important for use in a game that
    // runs over a very large timespan
    auto pool{PoolAllocator<Particle>(CAPACITY)};

    std::vector<Particle *> particles{};
    particles.reserve(CAPACITY);

    for (auto _: state) {
        // Create the memory
        for (size_t i = 0; i < CAPACITY; i++) {
            Particle *particle = pool.construct();

            if (particle == nullptr) {
                throw std::runtime_error("Pool system out of available objects, increase capacity!");
            }

            benchmark::DoNotOptimize(particle);

            particles.push_back(particle);
        }
        benchmark::ClobberMemory();

        // Then dump it all
        for (size_t i = 0; i < CAPACITY; i++) {
            Particle *particle = particles.back();

            particles.pop_back();

            pool.destroy(particle);
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * CAPACITY);
}

static void BM_NewParticleThroughput(benchmark::State &state) {
    std::vector<Particle *> particles{};
    particles.reserve(CAPACITY);

    for (auto _: state) {
        // Create the memory
        for (size_t i = 0; i < CAPACITY; i++) {
            auto particle = new Particle();

            benchmark::DoNotOptimize(particle);

            particles.push_back(particle);
        }
        benchmark::ClobberMemory();

        // Then dump it all
        for (size_t i = 0; i < CAPACITY; i++) {
            Particle *particle = particles.back();

            particles.pop_back();

            delete particle;
        }
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * CAPACITY);
}

BENCHMARK(BM_PoolParticleChurn);
BENCHMARK(BM_NewParticleChurn);

BENCHMARK(BM_PoolParticleThroughput);
BENCHMARK(BM_NewParticleThroughput);

BENCHMARK_MAIN();
