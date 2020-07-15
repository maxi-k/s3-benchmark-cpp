//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"

namespace benchmark::ram {
// --------------------------------------------------------------------------------
RunStats::RunStats(const RunParameters &params, const RunResults &run)
    : RunParameters(params)
    , samples_sum(params.thread_count * params.sample_count)
    , read_sum(params.payload_size * params.thread_count * params.sample_count)
    , latency(run.latencies, duration_t::zero())
    , duration(run.durations, duration_t::zero()) {
        this->bandwidth_gib_s = (1.0 * read_sum / units::gib) / (1.0 * duration.sum.count() / units::ns_per_sec);
}
// --------------------------------------------------------------------------------
}  // namespace benchmark::ram
