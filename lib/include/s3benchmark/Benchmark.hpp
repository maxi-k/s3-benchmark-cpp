//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include "Config.hpp"

namespace s3benchmark {
    using latency_t = size_t;
    using object_head_t = Aws::S3::Model::HeadObjectOutcome;

    struct Latency {
        latency_t first_byte;
        latency_t last_byte;
    };

    class Benchmark {
        const Config &config;
        Aws::S3::S3Client client;

    public:
        explicit Benchmark(const Config &config);
        void list_buckets();
        object_head_t fetch_object_head();
    };
}

#endif // _S3BENCHMARK_BENCHMARK_HPP
