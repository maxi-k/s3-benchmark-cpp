//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <thread>

#include "s3benchmark/Config.hpp"
#include "s3benchmark/Types.hpp"

namespace s3benchmark {

    Benchmark::Benchmark(const Config &config)
            : config(config)
            , client(Aws::S3::S3Client(config.aws_config())) {
    }

    void Benchmark::list_buckets() const {
        auto resp = client.ListBuckets();
        for (auto& bucket : resp.GetResult().GetBuckets()) {
            std::cout << "Found bucket: " << bucket.GetName() << std::endl;
        }
    }

    size_t Benchmark::fetch_object_size() const {
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

    Latency Benchmark::fetch_range(const ByteRange &range) const {
        // Create output buffer
        // std::vector<char> buf(range.length());
        auto req = Aws::S3::Model::GetObjectRequest()
                .WithBucket(config.bucket_name)
                .WithKey(config.object_name)
                .WithRange(range.as_http_header());
        // trash data after fetching
        req.SetResponseStreamFactory([]() {
            return Aws::New<Aws::FStream>("alloc_tag", "/dev/null", std::ios_base::out); });
        // Get Object, measure time to first byte.
        auto t_start = clock::now();
        auto resp = client.GetObject(req);
        auto t_first_byte  = clock::now();
        // Drain response body stream, measure last time byte when done
        auto res = resp.GetResultWithOwnership();
        // while (!res.GetBody().eof()) {
        //     res.GetBody().read(buf.data(), range.length());
        // }
        auto t_last_byte = clock::now();
        // Calculate and return first- and last byte latency
        return Latency {
            std::chrono::duration_cast<std::chrono::milliseconds>(t_first_byte - t_start),
            std::chrono::duration_cast<std::chrono::milliseconds>(t_last_byte - t_start)
        };
    }

    Latency Benchmark::fetch_random_range(size_t payload_size, size_t max_value) const {
        return fetch_range(random_range_in(payload_size, max_value));
    }

    ByteRange Benchmark::random_range_in(size_t size, size_t max_value) const {
        if (size > max_value) {
            throw std::runtime_error("Cannot create byte range larger than max size.");
        }
        auto offset = random::in_range<size_t>(0, max_value - size);
        return { offset, offset + size };
    }

    RunResults Benchmark::do_run(RunParameters &params) const {
        auto max_obj_size = this->fetch_object_size();
        auto per_thread_samples = std::max(params.sample_count / params.thread_count, params.thread_count);
        std::vector<Latency> results(per_thread_samples * params.thread_count);
        std::vector<std::thread> threads;

        clock::time_point start_time;
        bool do_start = false;

        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
           threads.emplace_back([this, t_id, per_thread_samples, max_obj_size, &params, &results, &do_start, &start_time]() {
               if (t_id != params.thread_count - 1) {
                   while (!do_start) { } // wait until all threads are started
               } else {
                   do_start = true; // the last started thread sets the start time
                   start_time = clock::now();
               }
               for (unsigned i = 0; i < per_thread_samples; ++i) {
                   auto idx = per_thread_samples * t_id + i;
                   results[idx] = this->fetch_random_range(params.payload_size, max_obj_size);
               }
           });
        }

        for (auto &thread : threads) {
            thread.join();
        }
        clock::time_point end_time = clock::now();
        return RunResults{
            results,
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time)
        };
    }

    void Benchmark::run_full_benchmark(Logger &logger) const {
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
                logger.print_run_results(params, results);
            }
            logger.print_run_footer();
        }
        // TODO: csv upload
    }
}  // namespace s3benchmark
