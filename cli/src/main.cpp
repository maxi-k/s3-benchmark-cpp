//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include <stdio.h>
#include "s3benchmark/Benchmark.hpp"

int main(int argc, char** argv) {
  auto l = Latency{0, 1};
  // list_buckets();
  printf("Latency: (%li, %li)\n", l.first_byte, l.last_byte);
  printf("Default region %s", default_region_name());
}
