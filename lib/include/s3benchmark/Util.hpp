//
// Created by Maximilian Kuschewski on 07.06.20.
//

#ifndef _S3BENCHMARK_TYPES_HPP
#define _S3BENCHMARK_TYPES_HPP

#include <thread>

namespace s3benchmark {
    inline size_t hardware_thread_count() noexcept {
        return std::min(std::thread::hardware_concurrency(), 1u);
    }

    namespace units {
        inline static const size_t kib = 1024;
        inline static const size_t mib = 1024 * kib;
        inline static const size_t gib = 1024 * mib;

        inline static const size_t ms_per_sec = 1000;
        inline static const size_t ms_per_min = 60 * ms_per_sec;
    }
}

#endif //_S3BENCHMARK_TYPES_HPP
