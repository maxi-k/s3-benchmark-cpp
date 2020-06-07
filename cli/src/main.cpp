//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"
#include "s3benchmark/cli/CliArgs.hpp"

using s3benchmark::Config;
using s3benchmark::Benchmark;
using s3benchmark::Latency;

int main(int argc, char** argv) {
    std::cout << "Hello World!\n";
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    auto config =  s3benchmark::cli::config_from_flags(&argc, &argv);
    auto b = Benchmark(config);
    auto l = Latency{0, 1};
    b.list_buckets();
    std::cout << "Latency: (" << l.first_byte << ", " << l.last_byte << ")" << std::endl;
    std::cout << "Default region is: " << config.region_name().data() << std::endl;
    b.fetch_object_head();

    std::flush(std::cout);
    Aws::ShutdownAPI(options);
    return 0;
}
