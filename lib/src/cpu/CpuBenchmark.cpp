//
// Created by maxi on 14.07.20.
//
#include "benchmark/cpu/CpuBenchmark.hpp"

namespace benchmark::cpu {
    // --------------------------------------------------------------------------------
    CpuBenchmark::CpuBenchmark(const CpuConfig &config)
        : config(config) { }
    // --------------------------------------------------------------------------------
    // make sure the measured operations are not optimized away by the compiler
#pragma GCC push_options
#pragma GCC optimize ("O0")
    // comparison + increment + N * (op + assignment)
    // ignoring jump b/c of branch prediction
    static const size_t OPS_PER_LOOP = 4;
    inline void do_iterations(size_t &iterations) {
        size_t tmp1 = 0;
        size_t tmp2 = 1ul << 27ul;
        for (size_t i = 0; i < iterations; ++i) { // 2 instructions
            tmp1 += tmp2; // 2 instructions
        }
    }
#pragma GCC pop_options
    // --------------------------------------------------------------------------------
    RunResults CpuBenchmark::do_run(const RunParameters &params) const {
        auto iterations = params.num_ops / OPS_PER_LOOP;
        std::vector<clock::duration> durations(params.samples);
        for (uint64_t n_sample = 0; n_sample < params.samples; ++n_sample) {
            auto t_start = clock::now();
            do_iterations(iterations);
            auto t_end = clock::now();
            durations[n_sample] = t_end - t_start;
        }
        return RunResults{ durations };
    }
    // --------------------------------------------------------------------------------
    void CpuBenchmark::run_full_benchmark(CpuLogger &logger) const {
        // TODO: extract into config variables
        auto params = RunParameters{ 1ul << 33ul, 5 };
        for (int i = 0; i < 3; ++i) {
            logger.print_run_params(params);
            logger.print_run_header();
            auto results = this->do_run(params);
            auto stats = RunStats(params, results);
            logger.print_run_stats(stats);
            logger.print_run_footer();

            params.samples *= 2;
            params.num_ops >>= 1ul;
        }
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cpu
