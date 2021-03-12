//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "benchmark/s3/S3Benchmark.hpp"
#include "benchmark/Util.hpp"
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <atomic>
#include <algorithm>
#include <thread>

namespace benchmark::s3 {

    S3Benchmark::S3Benchmark(const S3Config &config)
            : config(config)
            , client(Aws::S3::S3Client(config.aws_config())) {
    }

    void S3Benchmark::list_buckets() const {
        auto resp = client.ListBuckets();
        for (auto& bucket : resp.GetResult().GetBuckets()) {
            std::cout << "Found bucket: " << bucket.GetName() << std::endl;
        }
    }

    size_t S3Benchmark::fetch_object_size() const {
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

    latency_t S3Benchmark::fetch_range(const ByteRange &range, char* outbuf, size_t bufsize) const {
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
        return this->fetch_object(req, outbuf, bufsize);
    }

    [[nodiscard]] inline latency_t S3Benchmark::fetch_object(const Aws::S3::Model::GetObjectRequest &req, char* outbuf, size_t size) const {
        auto start = clock::now();
        auto outcome = client.GetObject(req);
        auto end = clock::now();
        if (outcome.IsSuccess()) {
            outcome.GetResultWithOwnership().GetBody().read(outbuf, size);
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    struct AsyncFetchContext : public Aws::Client::AsyncCallerContext {
            latency_t* target;
            Waiter& notifier;
            clock::time_point start_time;
        AsyncFetchContext(latency_t* target, Waiter& notifier)
            : AsyncCallerContext()
            , target(target)
            , notifier(notifier)
            , start_time(clock::now()) {};
    };

    inline void S3Benchmark::fetch_object_async(const Aws::S3::Model::GetObjectRequest &req, latency_t* target, char* outbuf, size_t length, Waiter& notifier) const {
      using Aws::Client::AsyncCallerContext;
      using Aws::S3::Model::GetObjectOutcome;
      using Aws::S3::Model::GetObjectRequest;
      using Aws::S3::S3Client;
      auto callback = [outbuf, length](
              [[maybe_unused]] const S3Client *async_client,
              [[maybe_unused]] const GetObjectRequest &async_request,
              GetObjectOutcome outcome,
              const std::shared_ptr<const AsyncCallerContext>& aws_context) {
          auto t_end = clock::now();
          if (!outcome.IsSuccess()) {
              auto err = outcome.GetError();
              std::cerr << "Error " << static_cast<int>(err.GetResponseCode()) << " while fetching: " << err.GetMessage() << std::endl;
              return;
          }
          auto ctx = reinterpret_cast<const AsyncFetchContext*>(aws_context.get());
          auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - ctx->start_time);
          // TODO: do something with result here
          *ctx->target = ms;
          outcome.GetResultWithOwnership().GetBody().read(outbuf, length);
          ctx->notifier.notify_done();
      };
      auto context = std::make_shared<AsyncFetchContext>(target, notifier);
      client.GetObjectAsync(req, callback, context);
    }

    ByteRange S3Benchmark::random_range_in(size_t size, size_t max_value) {
        if (size > max_value) {
            throw std::runtime_error("Cannot create byte range larger than max size.");
        }
        auto offset = random::in_range<size_t>(0, max_value - size);
        return { offset, offset + size };
    }

    RunResults S3Benchmark::do_run(RunParameters &params) const {
        auto max_obj_size = this->fetch_object_size();

        std::vector<char> outbuf(params.thread_count * params.payload_size);
        std::vector<latency_t> results(params.sample_count * params.thread_count);
        std::vector<std::thread> threads;

        std::vector<Aws::S3::Model::GetObjectRequest> requests;
        requests.reserve(params.sample_count * params.thread_count);
        for (size_t i = 0; i < params.sample_count * params.thread_count; ++i) {
            auto req = Aws::S3::Model::GetObjectRequest()
                    .WithBucket(config.bucket_name)
                    .WithKey(config.object_name)
                    .WithRange(random_range_in(params.payload_size, max_obj_size).as_http_header());
            // Put data into outbuf
            requests.push_back(req);
        }

        clock::time_point start_time;
        std::atomic<bool> do_start = false;

        std::vector<std::array<size_t, 256>> memspace(params.thread_count);
        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
           threads.emplace_back([this, t_id, max_obj_size, &requests, &params, &results, &do_start, &start_time, &outbuf]() {
               auto idx_start = params.sample_count * t_id;
               // Aws::IOStreamFactory stream_factory([&lspace]() {
               //   // Faster method than stringstream?
               //   // auto stream = Aws::New<Aws::StringStream>("S3Client", new MapBuf(lspace));
               //   // stream->rdbuf()->pubsetbuf(outbuf, bufsize);
               //   // return stream;
               //   return Aws::New<Aws::IOStream>("S3Client", new MapBuf(lspace));
               //   // return Aws::New<Aws::FStream>("S3Client", "/dev/null",
               //   // std::ios_base::out);
               // });
               struct Row{ size_t key; size_t value; };
               std::array<size_t, 16> hashmap;
               if (t_id != params.thread_count - 1) {
                   while (!do_start) { }  // wait until all threads are started
               } else {
                   do_start = true;  // the last started thread sets the start time
                   start_time = clock::now();
               }
               char* buf = outbuf.data() + t_id * params.payload_size;
               // //---- async ---- // 
               constexpr unsigned async_cnt = 4;
               Waiter waiter(async_cnt);
               for (unsigned i = 0; i < params.sample_count; ++i) {
                 // //---- sync ----//
                 // req.SetResponseStreamFactory(stream_factory);
                 auto diff = async_cnt - waiter.current();
                 for (unsigned j = 0; j != diff; ++j) {
                   this->fetch_object_async(requests[idx_start + i],
                                            &results[idx_start + i], buf,
                                            params.payload_size, waiter);
                   ++i;
                 }
                 waiter.wait_n(1);
                 // todo process async from extra buffer here
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

    void S3Benchmark::run_full_benchmark(S3Logger &logger) const {
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
}  // namespace benchmark::s3
