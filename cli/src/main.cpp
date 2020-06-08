//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"
#include "s3benchmark/cli/CliArgs.hpp"

using s3benchmark::Config;
using s3benchmark::Benchmark;
using s3benchmark::Latency;
namespace units = s3benchmark::units;

int main(int argc, char** argv) {
    std::cout << "Hello World!\n";
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    auto config = s3benchmark::cli::config_from_flags(&argc, &argv);
    auto b = Benchmark(config);

    std::cout << "Hardware thread count: " << s3benchmark::hardware::thread_count() << std::endl;
    b.list_buckets();
    std::cout << "Default region is: " << config.region_name().data() << std::endl;

    auto size = b.fetch_object_size();
    for (size_t i = 0; i < 64; ++i) {
        auto lat = b.fetch_random_range(1 * units::mib, size);
        std::cout << "Latency: (" << lat.first_byte << ", " << lat.last_byte << ")" << std::endl;
    }

    std::flush(std::cout);
    Aws::ShutdownAPI(options);
    return 0;
}
