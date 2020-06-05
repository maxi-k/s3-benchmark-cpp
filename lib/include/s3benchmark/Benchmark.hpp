//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP

#include <cstddef>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

namespace s3benchmark {
    using latency_t = size_t;
    using region_t = Aws::S3::Model::BucketLocationConstraint;
    using object_head_t = Aws::S3::Model::HeadObjectOutcome;

    const region_t default_region = region_t::eu_central_1;

    struct Latency {
        latency_t first_byte;
        latency_t last_byte;
    };

    class Benchmark {
        // Aws::S3::S3Client client;

    public:
        Benchmark();
        Aws::String region_name(const region_t &region = default_region);
        void list_buckets(const region_t &region = default_region);
        // object_head_t fetch_object_head(const std::string &object_name);
    };
}

#endif // _S3BENCHMARK_BENCHMARK_HPP
