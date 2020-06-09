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

    latency_t Benchmark::fetch_range(const ByteRange &range, char* outbuf, size_t bufsize) const {
        auto req = Aws::S3::Model::GetObjectRequest()
                .WithBucket(config.bucket_name)
                .WithKey(config.object_name)
                .WithRange(range.as_http_header());
        // Put data into outbuf
        req.SetResponseStreamFactory([&outbuf, &bufsize]() {
            // Faster method than stringstream?
            // auto stream = Aws::New<Aws::StringStream>("S3Client");
            // stream->rdbuf()->pubsetbuf(outbuf, bufsize);
            // return stream;
            return Aws::New<Aws::FStream>("S3Client", "/dev/null", std::ios_base::out);
        });
        auto start = clock::now();
        client.GetObject(req);
        auto end = clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    ByteRange Benchmark::random_range_in(size_t size, size_t max_value) {
        if (size > max_value) {
            throw std::runtime_error("Cannot create byte range larger than max size.");
        }
        auto offset = random::in_range<size_t>(0, max_value - size);
        return { offset, offset + size };
    }

    RunResults Benchmark::do_run(RunParameters &params) const {
        auto max_obj_size = this->fetch_object_size();

        std::vector<char> outbuf(params.thread_count * params.payload_size);
        std::vector<latency_t> results(params.sample_count * params.thread_count);
        std::vector<std::thread> threads;

        clock::time_point start_time;
        bool do_start = false;

        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
           threads.emplace_back([this, t_id, max_obj_size, &outbuf, &params, &results, &do_start, &start_time]() {
               auto buf = outbuf.data() + t_id * params.payload_size;
               auto range = random_range_in(params.payload_size, max_obj_size);
               auto idx_start = params.sample_count * t_id;

               if (t_id != params.thread_count - 1) {
                   while (!do_start) { } // wait until all threads are started
               } else {
                   do_start = true; // the last started thread sets the start time
                   start_time = clock::now();
               }
               for (unsigned i = 0; i < params.sample_count; ++i) {
                   results[idx_start + i] = this->fetch_range(range, buf, params.payload_size);
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
                auto stats = RunStats(params, results);
                logger.print_run_stats(stats);
            }
            logger.print_run_footer();
        }
        // TODO: csv upload
    }
}  // namespace s3benchmark
