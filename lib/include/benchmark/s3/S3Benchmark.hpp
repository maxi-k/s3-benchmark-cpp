//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _BENCHMARK_S3_BENCHMARK_HPP
#define _BENCHMARK_S3_BENCHMARK_HPP

#include <chrono>
#include <ratio>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include "benchmark/s3/S3Config.hpp"
#include "benchmark/s3/S3Logger.hpp"

namespace benchmark::s3 {
    using ObjectHead = Aws::S3::Model::HeadObjectOutcome;

    class S3Benchmark {
        using clock = std::chrono::steady_clock;

        const S3Config &config;
        Aws::S3::S3Client client;

    public:
        explicit S3Benchmark(const S3Config &config);

        void list_buckets() const;
        [[nodiscard]] size_t fetch_object_size() const;
        [[nodiscard]] latency_t fetch_range(const ByteRange &range, char* outbuf, size_t bufsize) const;
        [[nodiscard]] latency_t fetch_object(const Aws::S3::Model::GetObjectRequest &req) const;

        [[nodiscard]] static ByteRange random_range_in(size_t size, size_t max_value) ;
        [[nodiscard]] RunResults do_run(RunParameters &params) const;
        void run_full_benchmark(S3Logger &logger) const;
    };
}

#endif // _BENCHMARK_S3_BENCHMARK_HPP
