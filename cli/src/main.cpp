//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include <fstream>
#include "benchmark/s3/S3Benchmark.hpp"
#include "benchmark/s3/S3Config.hpp"
#include "benchmark/s3/S3Logger.hpp"
#include "benchmark/cli/CliArgs.hpp"

namespace benchmark::cli {
    int run_s3(Config &&bare_config, Logger &bare_logger) {
        Aws::SDKOptions options;
        Aws::InitAPI(options);
        auto config = s3::S3Config(std::move(bare_config));
        auto logger = s3::S3Logger(bare_logger);
        auto bm = s3::S3Benchmark(config);
        if (config.dry_run) {
            // TODO: print dry run
        } else {
            bm.run_full_benchmark(logger);
        }
        Aws::ShutdownAPI(options);
        return 0;
    }

    int run(int *argc, char ***argv) {
        auto config = config_from_flags(argc, argv);
        auto nullstream = std::ostream(nullptr);
        auto logger = Logger(config.quiet ? nullstream : std::cout);
        auto bench_type = get_parsed_bench_type();
        if (!bench_type.has_value()) {
            std::cerr << "Invalid benchmark type argument." << std::endl;
            return 1;
        }
        logger.print_config_params(config);
        logger.print_ec2_config(config);
        switch (bench_type.value()) {
            case S3:
                return run_s3(std::move(config), logger);
            case CPU: // TODO
            case MEMORY: // TODO
            case STORAGE: // TODO
                return 1;
        }
        return 0;
    }

}

int main(int argc, char** argv) {
    return benchmark::cli::run(&argc, &argv);
}
