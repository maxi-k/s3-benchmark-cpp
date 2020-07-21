//
// Created by maxi on 21.07.20.
//
#ifndef _BENCHMARK_CACHEBENCHMARK_HPP
#define _BENCHMARK_CACHEBENCHMARK_HPP

#include "benchmark/Config.hpp"
#include "benchmark/Logger.hpp"
#include "benchmark/Util.hpp"

namespace benchmark::cache {
    // --------------------------------------------------------------------------------
    using time_resolution_t = std::chrono::nanoseconds;
    using duration_t = std::chrono::duration<double, time_resolution_t::period>;
    // --------------------------------------------------------------------------------
    struct CacheConfig : Config {
        explicit CacheConfig(Config &&config) : Config(std::move(config)) { }
    };
    // --------------------------------------------------------------------------------
    struct RunParameters {
        size_t sample_count; // rep
        size_t thread_count;
        size_t payload_words; // n
        size_t linkset_jumps; // ool
    };
    // --------------------------------------------------------------------------------
    struct RunResults {
        std::vector<size_t> cycles;
        duration_t overall_duration;
        size_t read_sum_bytes;
        size_t computation_result;
    };
    // --------------------------------------------------------------------------------
    struct RunStats : RunParameters, RunResults {
        double bandwidth_gib_s;
        size_t samples_sum;
        ValueStats<size_t> cycles;
        RunStats(const RunParameters &params, const RunResults &results);
    };
    // --------------------------------------------------------------------------------
    struct CacheLogger: Logger, RunLogger<RunParameters, RunStats> {
        explicit CacheLogger(std::ostream &output) : Logger(output) {}
        explicit CacheLogger(Logger &logger) : Logger(logger) {}
        void print_run_header() const override;
        void print_run_footer() const override;
        void print_run_params(const RunParameters &params) const override;
        void print_run_stats(const RunStats &stats) const override;
    };
    // --------------------------------------------------------------------------------
    class CacheBenchmark {
        const CacheConfig &config;
    public:
        explicit CacheBenchmark(const CacheConfig &config);
        [[nodiscard]] RunResults do_run(const RunParameters &params, size_t* memspace) const;
        void run_full_benchmark(CacheLogger &logger) const;
    };
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cache
#endif //_BENCHMARK_CACHEBENCHMARK_HPP
