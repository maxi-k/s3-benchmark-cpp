//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include "Config.hpp"

namespace s3benchmark {
    using latency_t = long long int;
    using ObjectHead = Aws::S3::Model::HeadObjectOutcome;

    struct Latency {
        latency_t first_byte;
        latency_t last_byte;
    };

    struct ByteRange {
        size_t first_byte;
        size_t last_byte;

        inline std::string as_http_header() const {
            std::ostringstream s ;
            s << "bytes=" << first_byte << "-" << last_byte;
            return s.str();
        }

        inline size_t length() const {
            return last_byte - first_byte;
        }
    };

    class Benchmark {
        const Config &config;
        Aws::S3::S3Client client;

    public:
        explicit Benchmark(const Config &config);

        void list_buckets() const;
        size_t fetch_object_size() const;
        Latency fetch_range(const ByteRange &range) const;
        Latency fetch_random_range(size_t payload_size, size_t max_value) const;

        ByteRange random_range_in(size_t size, size_t max_value) const;

    };
}

#endif // _S3BENCHMARK_BENCHMARK_HPP
