//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP
#include <cstddef>

using latency_t = size_t;

struct Latency {
  latency_t first_byte;
  latency_t last_byte;
};

#endif // _S3BENCHMARK_BENCHMARK_HPP
