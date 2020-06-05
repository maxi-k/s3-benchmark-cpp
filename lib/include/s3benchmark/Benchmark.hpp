//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP

#include <cstddef>

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

using latency_t = size_t;

struct Latency {
  latency_t first_byte;
  latency_t last_byte;
};

const Aws::S3::Model::BucketLocationConstraint default_region = Aws::S3::Model::BucketLocationConstraint::eu_central_1;

const Aws::String default_region_name() {
  return Aws::S3::Model::BucketLocationConstraintMapper::GetNameForBucketLocationConstraint(default_region);
}


void list_buckets();

#endif // _S3BENCHMARK_BENCHMARK_HPP
