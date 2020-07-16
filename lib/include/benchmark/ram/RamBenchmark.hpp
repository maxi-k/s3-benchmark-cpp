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
    using byte_t = unsigned char;
    using time_resolution_t = std::chrono::nanoseconds;
    using duration_t = std::chrono::duration<double, time_resolution_t::period>;
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
        size_t payload_words;
    };
    // --------------------------------------------------------------------------------
    struct RunResults {
        std::vector<duration_t> durations;
        duration_t overall_duration;
        size_t bytes_read_sum;
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
    struct ThreadTaskParams {
        size_t thread_id;
        Barrier &barrier;
        size_t* tspace1;
        size_t* tspace2;
        duration_t* durations;
        size_t* read_count;
        size_t* result;
    };
    // --------------------------------------------------------------------------------
    class RamBenchmark {
        const RamConfig &config;

        static void thread_task_read(const RunParameters &params, ThreadTaskParams &t);
        static void thread_task_write(const RunParameters &params, ThreadTaskParams &t);
        static void thread_task_read_avx(const RunParameters &params, ThreadTaskParams &t);
    public:
        explicit RamBenchmark(const RamConfig &config);

        [[nodiscard]] RunResults do_run(const RunParameters &params, size_t* memspace1, size_t* memspace2) const;
        void run_full_benchmark(RamLogger &logger) const;
    };
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram

#endif //_BENCHMARK_RAMBENCHMARK_HPP
