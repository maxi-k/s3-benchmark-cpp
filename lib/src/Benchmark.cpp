//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>

#include "s3benchmark/Config.hpp"

namespace s3benchmark {

    Benchmark::Benchmark(const Config &config)
            : config(config)
            , client(Aws::S3::S3Client(config.aws_config())) {
    }

    void Benchmark::list_buckets() {
        auto resp = client.ListBuckets();
        for (auto& bucket : resp.GetResult().GetBuckets()) {
            std::cout << "Found bucket: " << bucket.GetName() << std::endl;
        }
    }

    object_head_t Benchmark::fetch_object_head() {
        auto req = Aws::S3::Model::HeadObjectRequest()
                .WithBucket(config.bucket_name)
                .WithKey(config.object_name);
        auto resp = client.HeadObject(req);
        auto len = resp.GetResult().GetContentLength();
        std::cout << "Content length: " << len << std::endl;
        return resp;
    }

}  // namespace s3benchmark
