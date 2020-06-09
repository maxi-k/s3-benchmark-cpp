//
// Created by maxi on 08.06.20.
//

#ifndef _S3BENCHMARK_TYPES_HPP
#define _S3BENCHMARK_TYPES_HPP

#include <chrono>
#include <ratio>
#include <vector>
#include <string>
#include <sstream>

namespace s3benchmark {
    using latency_t = std::chrono::duration<size_t, std::ratio<1, 1000>>;
    struct Latency {
        latency_t first_byte;
        latency_t last_byte;
    };

    struct ByteRange {
        size_t first_byte;
        size_t last_byte;

        inline std::string as_http_header() const {
            std::ostringstream s;
            s << "bytes=" << first_byte << "-" << last_byte;
            return s.str();
        }

        inline size_t length() const {
            return last_byte - first_byte;
        }
    };

    struct RunParameters {
        size_t sample_count;
        size_t thread_count;
        size_t payload_size;
    };

    struct RunResults {
        std::vector<latency_t> data_points;
        latency_t overall_time;
    };
}

#endif  // _S3BENCHMARK_TYPES_HPP
