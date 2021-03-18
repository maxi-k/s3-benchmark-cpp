//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "benchmark/s3/S3Benchmark.hpp"

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
#include <atomic>

namespace benchmark::s3 {
    // ----------------------------------------------------------------------------------------------------
    [[nodiscard]] inline latency_t fetch_object(const Aws::S3::S3Client& client, const Aws::S3::Model::GetObjectRequest &req) {
        auto start = clock::now();
        client.GetObject(req);
        auto end = clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }
    // ----------------------------------------------------------------------------------------------------
    template<>
    RunResults S3Benchmark<SYNC>::do_run(RunParameters &params) const {
        auto max_obj_size = this->fetch_object_size();
        std::vector<char> outbuf(params.thread_count * params.payload_size);
        std::vector<latency_t> results(params.sample_count * params.thread_count);
        std::vector<std::thread> threads;

        std::vector<Aws::S3::Model::GetObjectRequest> requests;
        requests.reserve(params.sample_count * params.thread_count);
        Aws::IOStreamFactory stream_factory([]() {
            // Faster method than stringstream?
            // auto stream = Aws::New<Aws::StringStream>("S3Client");
            // stream->rdbuf()->pubsetbuf(outbuf, bufsize);
            // return stream;
            return Aws::New<Aws::FStream>("S3Client", "/dev/null", std::ios_base::out);
        });
        for (size_t i = 0; i < params.sample_count * params.thread_count; ++i) {
            auto req = Aws::S3::Model::GetObjectRequest()
                    .WithBucket(config.bucket_name)
                    .WithKey(config.object_name)
                    .WithRange(random_range_in(params.payload_size, max_obj_size).as_http_header());
            // Put data into outbuf
            req.SetResponseStreamFactory(stream_factory);
            requests.push_back(req);
        }

        clock::time_point start_time;
        std::atomic<bool> do_start = false;

        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
           threads.emplace_back([this, t_id, max_obj_size, &requests, &params, &results, &do_start, &start_time]() {
               auto idx_start = params.sample_count * t_id;

               if (t_id != params.thread_count - 1) {
                   while (!do_start) { }  // wait until all threads are started
               } else {
                   do_start = true;  // the last started thread sets the start time
                   start_time = clock::now();
               }
               for (unsigned i = 0; i < params.sample_count; ++i) {
                   results[idx_start + i] = fetch_object(client, requests[idx_start + i]);
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
    // ----------------------------------------------------------------------------------------------------
}  // namespace benchmark::s3
