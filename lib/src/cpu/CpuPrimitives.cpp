//
// Created by maxi on 14.07.20.
//

#ifndef _BENCHMARK_CPUPRIMITIVES_HPP
#define _BENCHMARK_CPUPRIMITIVES_HPP
#include "benchmark/cpu/CpuBenchmark.hpp"
#include "benchmark/Util.hpp"
#include <chrono>

namespace benchmark::cpu {
    // --------------------------------------------------------------------------------
    RunStats::RunStats(const RunParameters &params, const RunResults &results)
        : RunParameters(params)
        , RunResults(results)
        , total_ops(params.num_ops * params.samples)
        , total_duration(0) {
            for (auto &sample : this->exec_times) {
                this->total_duration += std::chrono::duration_cast<std::chrono::microseconds>(sample).count() * 1.0 / units::mys_per_sec;
            }
            this->clock_speed = total_ops * 1.0 / this->total_duration / units::giga;
        }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cpu

#endif //_BENCHMARK_CPUPRIMITIVES_HPP
