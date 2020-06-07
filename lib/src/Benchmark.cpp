//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>

#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include "s3benchmark/Config.hpp"

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
        auto len = resp.GetResult().GetContentLength();
        return len;
    }

    Latency Benchmark::fetch_range(const ByteRange &range) const {
        auto req = Aws::S3::Model::GetObjectRequest()
                .WithBucket(config.bucket_name)
                .WithKey(config.object_name)
                .WithRange(range.as_http_header());
        auto t_start = std::chrono::steady_clock::now();
        auto res = client.GetObject(req).GetResultWithOwnership();
        auto t_first_byte  = std::chrono::steady_clock::now();
        auto length = range.length();
        auto read_buf = std::vector<char>(length);
        while (!res.GetBody().eof()) {
            res.GetBody().read(read_buf.data(), length);
        }
        auto t_last_byte = std::chrono::steady_clock::now();
        return Latency {
            std::chrono::duration_cast<std::chrono::milliseconds>(t_first_byte - t_start).count(),
            std::chrono::duration_cast<std::chrono::milliseconds>(t_last_byte - t_start).count()
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

}  // namespace s3benchmark
