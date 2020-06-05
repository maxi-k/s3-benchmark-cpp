//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include <cstdio>
#include "s3benchmark/Benchmark.hpp"

using namespace s3benchmark;

int main(int argc, char** argv) {
    auto b = Benchmark();
    auto l = Latency{0, 1};
    b.list_buckets();
    printf("Latency: (%li, %li)\n", l.first_byte, l.last_byte);
    printf("Default region is %s", b.region_name().data());
}
