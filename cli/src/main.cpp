//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "benchmark/s3/S3Benchmark.hpp"
#include "benchmark/cli/CliArgs.hpp"
#include "benchmark/cli/CliLogger.hpp"

namespace benchmark::cli {
    void run_s3(const benchmark::Config &config) {
        auto bm = s3::S3Benchmark(config);
        auto void_log = Logger();
        auto cli_log = CliLogger(std::cout);
        cli_log.print_config_params(config);
        cli_log.print_ec2_config(config);
        if (config.dry_run) {
            // TODO: print dry run
        } else {
            bm.run_full_benchmark(config.quiet ? void_log : cli_log);
        }
    }
}

int main(int argc, char** argv) {
    // Initialize AWS SDK
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    // Read config, run benchmark
    auto config = benchmark::cli::config_from_flags(&argc, &argv);
    benchmark::cli::run_s3(config);
    // Shutdown API, flush cout
    Aws::ShutdownAPI(options);
    return 0;
}
