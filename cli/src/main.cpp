//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"
#include "s3benchmark/cli/CliArgs.hpp"
#include "s3benchmark/cli/CliLogger.hpp"

namespace s3benchmark::cli {
    void run(const s3benchmark::Config &config) {
        auto bm = Benchmark(config);
        auto void_log = Logger();
        auto cli_log = CliLogger(std::cout);
        cli_log.print_config_params(config);
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
    auto config = s3benchmark::cli::config_from_flags(&argc, &argv);
    s3benchmark::cli::run(config);
    // Shutdown API, flush cout
    Aws::ShutdownAPI(options);
    return 0;
}
