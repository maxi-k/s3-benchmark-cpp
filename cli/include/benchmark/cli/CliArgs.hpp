//
// Created by Maximilian Kuschewski on 2020-05-06
//
#ifndef _BENCHMARK_CLIARGS_HPP
#define _BENCHMARK_CLIARGS_HPP

#include <thread>
#include <exception>

#include <gflags/gflags.h>
#include "benchmark/Config.hpp"
#include "benchmark/s3/S3Config.hpp"

namespace benchmark::cli {

    enum BenchType {
        S3, CPU, MEMORY, STORAGE
    };
    const static char* BENCH_TYPES[] = { "s3", "cpu", "memory", "storage" };

    bool validate_bench(const char* flagname, const std::string &value) {
        for (auto& type : BENCH_TYPES) {
            if (value == type) {
                return true;
            }
        }
        return false;
    }

    #pragma clang diagnostic push
    #pragma ide diagnostic ignored "cert-err58-cpp"
    // Define program arguments using gflags macros
    namespace flags {
        DEFINE_string(bench, "s3", "Which benchmark to run. Options are s3, cpu, memory, storage.");
        DEFINE_validator(bench, &validate_bench);

        DEFINE_bool(quiet, false, "If true, log run results etc. to the cli.");
        DEFINE_bool(dry_run, false, "If true, don't do any requests but do a dry run instead");
        DEFINE_bool(threads_static, false,
                    "If true, interprete threads-min and threads-max as static counts instead of multiples of the hardware thread count.\n"
                    "It's advised to explicitly set threads-min and threads-max if this option is given.");
        DEFINE_double(threads_min, 16, // 1
                      "The minimum number of threads to use when fetching objects from S3 as a multiple of the hardware thread count.");
        DEFINE_double(threads_max, 32, // 2
                      "The maximum number of threads to use when fetching objects from S3 as a multiple of the hardware thread count.");
        DEFINE_double(threads_step, 2,
                      "What increase in thread count per benchmark run is. Positive means multiplicative, negative means additive.");
        DEFINE_uint64(payloads_min, 10 * units::kib,
                      "The minimum object size to test, with 1 = 1 MB, and every increment is a double of the previous value.");
        DEFINE_uint64(payloads_max, 160 * units::kib,
                      "The maximum object size to test, with 1 = 1 MB, and every increment is a double of the previous value.");
        DEFINE_uint64(payloads_step, 2,
                      "What the multiplicative increase in payload size per benchmark run is (size *= step). Must be > 1");
        DEFINE_bool(payloads_reverse, false,
                    "If true, start with the largest payload size first and decrease from there");
        DEFINE_uint64(samples, 10,
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

    } // namespace flags

    Config config_from_flags(int *argc, char ***argv) {
        gflags::ParseCommandLineFlags(argc, argv, true);
        if (*argc > 1) {
            throw std::runtime_error("More arguments given than necessary. Quitting.");
        }
        return Config({
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
                      });
    }

    std::optional<BenchType> get_parsed_bench_type() {
        auto flag = flags::FLAGS_bench;
        for (unsigned i = 0; i < sizeof(BENCH_TYPES); ++i) {
            if (flag == BENCH_TYPES[i])   {
                return static_cast<BenchType>(i);
            }
        }
        return {};
    }

}  // namespace benchmark

#endif // _BENCHMARK_CLIARGS_HPP

#pragma clang diagnostic pop