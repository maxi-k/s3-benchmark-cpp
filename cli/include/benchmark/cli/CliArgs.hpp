//
// Created by Maximilian Kuschewski on 2020-05-06
//
#ifndef _BENCHMARK_CLIARGS_HPP
#define _BENCHMARK_CLIARGS_HPP

#include <thread>
#include <exception>
#include <optional>
#include <assert.h>
#include <vector>
#include <initializer_list>

#include <gflags/gflags.h>
#include "benchmark/Config.hpp"
#include "benchmark/s3/S3Benchmark.hpp"

namespace benchmark::cli {

    template<typename EnumType, typename Value, auto N, const char* (&Set)[N]>
    struct EnumParser {
        static std::optional<EnumType> member_to_enum(const Value &value) {
            for (unsigned i = 0; i < N; ++i) {
                if (Set[i] == value) {
                    return static_cast<EnumType>(i);
                }
            }
            return std::nullopt;
        }

        static bool validate([[maybe_unused]] const char* flagname, const Value &value) {
            return member_to_enum(value).has_value();
        }
    };

    using BENCH_TYPE_PARSER = EnumParser<BenchType, std::string, 4, BENCH_TYPE_NAMES>;
    using IO_MODE_PARSER = EnumParser<IOMode, std::string, 2, IO_MODE_NAMES>;
    using RAM_MODE_PARSER = EnumParser<RamTestMode, std::string, 3, RAM_MODE_NAMES>;

    #pragma clang diagnostic push
    #pragma ide diagnostic ignored "cert-err58-cpp"
    // Define program arguments using gflags macros
    namespace flags {
        DEFINE_string(bench, "cache", "Which benchmark to run. Options are [s3], [cpu], [memory], [storage].");
        DEFINE_validator(bench, BENCH_TYPE_PARSER::validate);

        DEFINE_bool(quiet, false, "If true, log run results etc. to the cli.");
        DEFINE_bool(dry_run, false, "If true, don't do any requests but do a dry run instead");
        DEFINE_bool(threads_static, false,
                    "If true, interprete threads-min and threads-max as static counts instead of multiples of the hardware thread count.\n"
                    "It's advised to explicitly set threads-min and threads-max if this option is given.");
        DEFINE_double(threads_min, 1, // 1
                      "The minimum number of threads to use when fetching objects from S3 as a multiple of the hardware thread count.");
        DEFINE_double(threads_max, 16, // 2
                      "The maximum number of threads to use when fetching objects from S3 as a multiple of the hardware thread count.");
        DEFINE_double(threads_step, 2,
                      "The increase in thread count per benchmark run, multiplicative (n *= threads_step)");
        DEFINE_uint64(payloads_min, 1 * units::kib,
                      "The minimum object size to test, with 1 = 1 MB, and every increment is a double of the previous value.");
        DEFINE_uint64(payloads_max, 128 * units::mib,
                      "The maximum object size to test, with 1 = 1 MB, and every increment is a double of the previous value.");
        DEFINE_uint64(payloads_step, 2,
                      "What the multiplicative increase in payload size per benchmark run is (size *= step). Must be > 1");
        DEFINE_bool(payloads_reverse, false,
                    "If true, start with the largest payload size first and decrease from there");
        DEFINE_uint64(samples, 1000000,
                      "The number of samples to collect for each test of a single object size per thread.");
        DEFINE_uint64(samples_cap, 7200,
                      "The maximum number of samples to collect for each test of a single object size.");
        DEFINE_string(bucket_name, s3::S3Config::DEFAULT_BUCKET_NAME,
                      "The name of the bucket where the test object is located");
        DEFINE_string(object_name, s3::S3Config::DEFAULT_OBJECT_NAME,
                      "The name of the large object file where data will be fetched from");
        DEFINE_string(region, s3::S3Config::DEFAULT_REGION,
                      "Sets the AWS region to use for the S3 bucket. Only applies if the bucket doesn't already exist.");
        DEFINE_bool(full, false,
                    "Runs the full exhaustive test, and overrides the threads and payload arguments.");
        DEFINE_bool(throttling_mode, false,
                    "Runs a continuous test to find out when EC2 network throttling kicks in.");
        DEFINE_string(upload_csv, "",
                      "Uploads the test results to S3 as a CSV file.");
        DEFINE_string(upload_stats, "",
                      "Upload CPU stats from during the benchmark to S3 as a CSV file.");

        DEFINE_string(io_mode, "sync", "Which I/O strategy to use. Options are [sync] and [uring]");
        DEFINE_validator(io_mode, IO_MODE_PARSER::validate);

        DEFINE_string(ram_mode, "read",
                      #ifdef __AVX2__
                              "Whether to test [read] or [write] or [read-avx] speed for the RAM benchmark."
#else
                "Whether to test [read] or [write] speed for the RAM benchmark."
#endif
                      );
        DEFINE_validator(ram_mode, RAM_MODE_PARSER::validate);

        DEFINE_uint64(cache_reads_min, 128, "The minimum number of reads for each repetition in the cache benchmark.");
        DEFINE_uint64(cache_reads_max, 128, "The maximum number of reads for each repetition in the cache benchmark.");
        DEFINE_uint64(cache_reads_step, 2, "The multiplier for each cache read count.");

    } // namespace flags

    Config config_from_flags(int *argc, char ***argv) {
        gflags::ParseCommandLineFlags(argc, argv, true);
        if (*argc > 1) {
            throw std::runtime_error("More arguments given than necessary. Quitting.");
        }
        return Config({
                              BENCH_TYPE_PARSER::member_to_enum(flags::FLAGS_bench).value(),
                              flags::FLAGS_dry_run,
                              flags::FLAGS_quiet,
                              flags::FLAGS_threads_static,
                              flags::FLAGS_threads_min,
                              flags::FLAGS_threads_max,
                              flags::FLAGS_threads_step,
                              flags::FLAGS_payloads_min,
                              flags::FLAGS_payloads_max,
                              flags::FLAGS_payloads_step,
                              flags::FLAGS_payloads_reverse,
                              flags::FLAGS_samples,
                              flags::FLAGS_samples_cap,
                              flags::FLAGS_bucket_name,
                              flags::FLAGS_object_name,
                              flags::FLAGS_region,
                              flags::FLAGS_full,
                              flags::FLAGS_throttling_mode,
                              flags::FLAGS_upload_csv,
                              flags::FLAGS_upload_stats,
                              IO_MODE_PARSER::member_to_enum(flags::FLAGS_io_mode).value(),
                              RAM_MODE_PARSER::member_to_enum(flags::FLAGS_ram_mode).value(),
                              flags::FLAGS_cache_reads_min,
                              flags::FLAGS_cache_reads_max,
                              flags::FLAGS_cache_reads_step,
                      });
    }

}  // namespace benchmark::cli

#endif // _BENCHMARK_CLIARGS_HPP

#pragma clang diagnostic pop
