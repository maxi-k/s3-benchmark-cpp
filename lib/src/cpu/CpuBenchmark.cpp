//
// Created by maxi on 14.07.20.
//
#include "benchmark/cpu/CpuBenchmark.hpp"

namespace benchmark::cpu {
    // --------------------------------------------------------------------------------
    CpuBenchmark::CpuBenchmark(const CpuConfig &config)
        : config(config) { }
    // --------------------------------------------------------------------------------
    RunResults CpuBenchmark::do_run(const RunParameters &params) const {
        static const uint64_t OPS_PER_LOOP = 10;
        auto iterations = params.num_ops / OPS_PER_LOOP;
        std::vector<clock::duration> durations(params.samples);
        for (uint64_t n_sample = 0; n_sample < params.samples; ++n_sample) {
            uint64_t ops_tmp1 = 0;
            uint64_t ops_tmp2 = 100;
            auto t_start = clock::now();
            for (uint64_t i = 0; i < iterations; ++i) {
                ops_tmp1 += ops_tmp2;
                ops_tmp1 += ops_tmp1;
                ops_tmp2 += 33;
                ops_tmp1 -= ops_tmp2;
            }
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
