//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
#include "benchmark/Util.hpp"

namespace benchmark::ram {
// --------------------------------------------------------------------------------
RunStats::RunStats(const RunParameters &params, const RunResults &run)
    : RunParameters(params)
    , read_sum(run.bytes_read_sum)
    , samples_sum(params.thread_count * params.sample_count)
    , duration(run.durations, duration_t::zero()) {
        this->bandwidth_gib_s = read_sum * (1.0 * units::ns_per_sec / units::gib) / run.overall_duration.count();
}
// --------------------------------------------------------------------------------
}  // namespace benchmark::ram
