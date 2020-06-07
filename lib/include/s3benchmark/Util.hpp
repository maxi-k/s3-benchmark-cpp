//
// Created by Maximilian Kuschewski on 07.06.20.
//

#ifndef _S3BENCHMARK_TYPES_HPP
#define _S3BENCHMARK_TYPES_HPP

#include <thread>
#include <random>

namespace s3benchmark {
    namespace hardware {
        inline size_t thread_count() noexcept {
            return std::min(std::thread::hardware_concurrency(), 1u);
        }
    }

    namespace units {
        inline static const size_t kib = 1024;
        inline static const size_t mib = 1024 * kib;
        inline static const size_t gib = 1024 * mib;

        inline static const size_t ms_per_sec = 1000;
        inline static const size_t ms_per_min = 60 * ms_per_sec;
    }

    namespace random {
        template<typename T>
        inline T in_range(T min, T max) {
            thread_local static std::mt19937 engine {std::random_device{}()};
            thread_local static std::uniform_int_distribution<T> dist(min, max);
            return dist(engine);
        };
    }
}

#endif //_S3BENCHMARK_TYPES_HPP
