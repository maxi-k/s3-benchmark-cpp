//
// Created by maxi on 14.07.20.
//

#ifndef _BENCHMARK_CPUBENCHMARK_H
#define _BENCHMARK_CPUBENCHMARK_H

#include <benchmark/Logger.hpp>
#include "benchmark/Config.hpp"

namespace benchmark::cpu {
    class CpuConfig : public Config {
    public:
        explicit CpuConfig(Config &&config);
    };
    // --------------------------------------------------------------------------------
    struct RunParameters {
        uint64_t num_ops;
        uint64_t samples;
    };
    // --------------------------------------------------------------------------------
    struct RunResults {
        std::vector<clock::duration> exec_times;
    };
    // --------------------------------------------------------------------------------
    struct RunStats : RunResults, RunParameters {
        size_t total_ops;
        double total_duration;
        double clock_speed;
        RunStats(const RunParameters &params, const RunResults &results);
    };
    // --------------------------------------------------------------------------------
    class CpuLogger: public Logger, public RunLogger<RunParameters, RunStats> {
    public:
        explicit CpuLogger(std::ostream &output) : Logger(output) {}
        explicit CpuLogger(Logger &logger) : Logger(logger) {}
        void print_run_header() const override;
        void print_run_footer() const override;
        void print_run_params(const RunParameters &params) const override;
        void print_run_stats(const RunStats &stats) const override;
    };
    // --------------------------------------------------------------------------------
    class CpuBenchmark {
        const CpuConfig &config;
    public:
        explicit CpuBenchmark(const CpuConfig &config);
        [[nodiscard]] RunResults do_run(const RunParameters &params) const;
        void run_full_benchmark(CpuLogger &logger) const;
    };
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cpu
#endif //_BENCHMARK_CPUBENCHMARK_H
