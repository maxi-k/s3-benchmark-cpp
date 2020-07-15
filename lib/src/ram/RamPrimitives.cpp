//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
#include "benchmark/Util.hpp"

namespace benchmark::ram {
// --------------------------------------------------------------------------------
RunStats::RunStats(const RunParameters &params, const RunResults &run)
    : RunParameters(params)
    , samples_sum(params.thread_count * params.sample_count)
    , read_sum(params.payload_size * params.thread_count * params.sample_count)
    , duration(run.durations, duration_t::zero()) {
        this->bandwidth_gib_s = (1.0 * read_sum / units::gib) / units::duration_seconds(run.overall_duration).count();
}
// --------------------------------------------------------------------------------
}  // namespace benchmark::ram
