//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include <fstream>
#include <utility>

#include "benchmark/s3/S3Benchmark.hpp"
#include "benchmark/cpu/CpuBenchmark.hpp"
#include "benchmark/ram/RamBenchmark.hpp"
#include "benchmark/cache/CacheBenchmark.hpp"
#include "benchmark/cli/CliArgs.hpp"

namespace benchmark::cli {
    // --------------------------------------------------------------------------------
    int run_s3(Config &&bare_config, Logger &bare_logger) {
        Aws::SDKOptions options;
        Aws::InitAPI(options);
        auto config = s3::S3Config(std::move(bare_config));
        auto logger = s3::S3Logger(bare_logger);
        if (config.dry_run) {
          // TODO: print dry run
          return 0;
        }
        if (config.io_mode == SYNC) {
          auto bm = s3::S3Benchmark<SYNC>(config);
          bm.run_full_benchmark(logger);
        } else {
          auto bm = s3::S3Benchmark<URING>(config);
          bm.run_full_benchmark(logger);
        }
        Aws::ShutdownAPI(options);
        return 0;
    }
    // --------------------------------------------------------------------------------
    int run_cpu(Config &&bare_config, Logger &bare_logger) {
        auto config = cpu::CpuConfig(std::move(bare_config));
        auto logger = cpu::CpuLogger(bare_logger);
        auto bm = cpu::CpuBenchmark(config);
        bm.run_full_benchmark(logger);
        return 0;
    }
    // --------------------------------------------------------------------------------
    int run_ram(Config &&bare_config, Logger &bare_logger) {
        auto config = ram::RamConfig(std::move(bare_config));
        auto logger = ram::RamLogger(bare_logger);
        auto bm = ram::RamBenchmark(config);
        bm.run_full_benchmark(logger);
        return 0;
    }
    // --------------------------------------------------------------------------------
    int run_cache(Config &&bare_config, Logger &bare_logger) {
        auto config = cache::CacheConfig(std::move(bare_config));
        auto logger = cache::CacheLogger(bare_logger);
        auto bm = cache::CacheBenchmark(config);
        bm.run_full_benchmark(logger);
        return 0;
    }
    // --------------------------------------------------------------------------------
    using runner_t = int (*)(Config &&bar_config, Logger &bare_logger);
    static constexpr runner_t bench_runners[4] = {
            [S3] = run_s3,
            [CPU] = run_cpu,
            [RAM] = run_ram,
            [CACHE] = run_cache
    };
    // --------------------------------------------------------------------------------
    int run(int *argc, char ***argv) {
        auto config = config_from_flags(argc, argv);
        auto nullstream = std::ostream(nullptr);
        auto logger = Logger(config.quiet ? nullstream : std::cout);
        logger.print_ec2_config(config);
        logger.print_config_params(config);
        return bench_runners[config.bench_type](std::move(config), logger);
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cli
// --------------------------------------------------------------------------------
int main(int argc, char** argv) {
    return benchmark::cli::run(&argc, &argv);
}
// --------------------------------------------------------------------------------
