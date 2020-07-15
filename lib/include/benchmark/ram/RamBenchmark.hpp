//
// Created by maxi on 15.07.20.
//
#ifndef _BENCHMARK_RAMBENCHMARK_HPP
#define _BENCHMARK_RAMBENCHMARK_HPP

#include "benchmark/Config.hpp"
#include "benchmark/Logger.hpp"

#include <vector>
#include <chrono>

namespace benchmark::ram {
    // --------------------------------------------------------------------------------
    using time_resolution_t = std::chrono::nanoseconds;
    using duration_t = std::chrono::duration<size_t, time_resolution_t::period>;
    using second_t = std::chrono::duration<double>;
    // --------------------------------------------------------------------------------
    struct RamConfig : Config {
        size_t cache_size;
        explicit RamConfig(Config &&config);
    };
    // --------------------------------------------------------------------------------
    struct RunParameters {
        size_t sample_count;
        size_t thread_count;
        size_t payload_size;
    };
    // --------------------------------------------------------------------------------
    struct RunResults {
        std::vector<duration_t> durations;
        duration_t overall_duration;
    };
    // --------------------------------------------------------------------------------
    struct RunStats : RunParameters {
        double bandwidth_gib_s;
        size_t read_sum;
        size_t samples_sum;
        ValueStats<duration_t> duration;
        RunStats(const RunParameters &params, const RunResults &results);
    };
    // --------------------------------------------------------------------------------
    struct RamLogger : Logger, RunLogger<RunParameters, RunStats> {
        explicit RamLogger(Logger &logger) : Logger(logger) {}
        void print_run_header() const override;
        void print_run_footer() const override;
        void print_run_params(const RunParameters &params) const override;
        void print_run_stats(const RunStats &stats) const override;
    };
    // --------------------------------------------------------------------------------
    class RamBenchmark {
        const RamConfig &config;
    public:
        explicit RamBenchmark(const RamConfig &config);
        [[nodiscard]] RunResults do_run(const RunParameters &params, std::vector<char> *memspace1, std::vector<char> *memspace2) const;
        void run_full_benchmark(RamLogger &logger) const;
    };
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram

#endif //_BENCHMARK_RAMBENCHMARK_HPP
