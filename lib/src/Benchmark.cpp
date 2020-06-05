//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"

#include <cstdio>

using namespace s3benchmark;

Benchmark::Benchmark() {
}

void Benchmark::list_buckets(const region_t &region) {
  printf("Listing buckets in region %s\n");
}

Aws::String Benchmark::region_name(const region_t &region) {
    return Aws::S3::Model::BucketLocationConstraintMapper::GetNameForBucketLocationConstraint(region);
}

// object_head_t Benchmark::fetch_object_head(const std::string &object_name) {
//     auto req = Aws::S3::Model::HeadObjectRequest()
// }
