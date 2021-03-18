//
// Created by Maximilian Kuschewski on 2021-03-18
//
#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/HeadObjectRequest.h>

#include "benchmark/Config.hpp"
#include "benchmark/s3/S3Benchmark.hpp"

namespace benchmark::s3 {
    // ----------------------------------------------------------------------------------------------------
    template<IOMode Mode>
    S3Benchmark<Mode>::S3Benchmark(const S3Config &config)
            : config(config)
            , client(Aws::S3::S3Client(config.aws_config())) {
    }
    // ----------------------------------------------------------------------------------------------------
    template<IOMode Mode>
    void S3Benchmark<Mode>::list_buckets() const {
        auto resp = client.ListBuckets();
        for (auto& bucket : resp.GetResult().GetBuckets()) {
            std::cout << "Found bucket: " << bucket.GetName() << std::endl;
        }
    }
    // ----------------------------------------------------------------------------------------------------
    template<IOMode Mode>
    size_t S3Benchmark<Mode>::fetch_object_size() const {
        auto req = Aws::S3::Model::HeadObjectRequest()
                .WithBucket(config.bucket_name)
                .WithKey(config.object_name);
        auto resp = client.HeadObject(req);
        if (!resp.IsSuccess()) {
            throw std::runtime_error("Could not fetch object head.");
        }
        auto len = resp.GetResult().GetContentLength();
        return len;
    }
    // ----------------------------------------------------------------------------------------------------
    template<IOMode Mode>
    ByteRange S3Benchmark<Mode>::random_range_in(size_t size, size_t max_value) {
        if (size > max_value) {
            throw std::runtime_error("Cannot create byte range larger than max size.");
        }
        auto offset = random::in_range<size_t>(0, max_value - size);
        return { offset, offset + size };
    }
    // ----------------------------------------------------------------------------------------------------
    template<IOMode Mode>
    void S3Benchmark<Mode>::run_full_benchmark(S3Logger &logger) const {
        // TODO: consider config.payloads_step
        auto params = RunParameters{ config.samples, 1, 0 };
        for (size_t payload_size = config.payloads_min; payload_size <= config.payloads_max; payload_size *= 2) {
            params.payload_size = payload_size;
            logger.print_run_params(params);
            logger.print_run_header();
            // TODO: consider config.threads_step
            for (size_t thread_count = config.threads_min; thread_count <= config.threads_max; thread_count *= 2) {
                params.thread_count =  thread_count;
                auto results = this->do_run(params);
                auto stats = RunStats(params, results);
                logger.print_run_stats(stats);
            }
            logger.print_run_footer();
        }
        // TODO: csv upload
    }
    // ----------------------------------------------------------------------------------------------------
}  // namespace benchmark::s3
