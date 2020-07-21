//
// Created by maxi on 21.07.20.
//
#include "benchmark/cache/CacheBenchmark.hpp"

namespace benchmark::cache {
    // --------------------------------------------------------------------------------
    RunStats::RunStats(const RunParameters &params, const RunResults &run)
            : RunParameters(params)
            , RunResults(run)
            , samples_sum(params.thread_count * params.sample_count)
            , cycles(run.cycles, 0ul) {
        this->bandwidth_gib_s =  read_sum_bytes * (1.0 * units::ns_per_sec / units::gib) / run.overall_duration.count();
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cache
