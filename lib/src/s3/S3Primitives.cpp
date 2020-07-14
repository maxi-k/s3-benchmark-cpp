//
// Created by maxi on 14.07.20.
//
#include "benchmark/s3/S3Benchmark.hpp"
namespace benchmark::s3 {
    // --------------------------------------------------------------------------------
    RunStats::RunStats(const RunParameters &params, const RunResults &run)
            : RunParameters(params), samples_sum(run.data_points.size()), duration(run.overall_time) {
        latency_t l_min, l_max = run.data_points[0];
        latency_t l_sum = latency_t::zero();
        for (auto &dp : run.data_points) {
            if (dp < l_min) l_min = dp;
            if (dp > l_max) l_max = dp;
            l_sum += dp;
        }
        latency_t l_avg = l_sum / this->samples_sum;
        this->download_sum = samples_sum * params.payload_size;
        this->throughput_mbps = (download_sum * 1.0 / units::mib) / (duration.count() * 1.0 / units::ms_per_sec);
        this->latency_avg = l_avg;
        this->latency_min = l_min;
        this->latency_max = l_max;
        this->latency_sum = l_sum;
    }
    // --------------------------------------------------------------------------------
    std::string ByteRange::as_http_header() const {
        std::ostringstream s;
        s << "bytes=" << first_byte << "-" << last_byte;
        return s.str();
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::s3
