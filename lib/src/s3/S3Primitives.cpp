//
// Created by maxi on 14.07.20.
//
#include <string>
#include "benchmark/s3/S3Benchmark.hpp"
namespace benchmark::s3 {
    // --------------------------------------------------------------------------------
    latency_t to_latency(const request_time_t& time) {
        return std::chrono::duration_cast<std::chrono::microseconds>(time.second - time.first);
    };
    // --------------------------------------------------------------------------------
    RunStats::RunStats(const RunParameters &params, const RunResults &run)
            : RunParameters(params)
            , samples_sum(run.data_points.size())
            , download_sum(samples_sum * params.payload_size)
            , duration(run.overall_time) {
        latency_t l_min = to_latency(run.data_points[0]);
        latency_t l_max = to_latency(run.data_points[0]);
        latency_t l_sum = latency_t::zero();
        for (auto &dp_t : run.data_points) {
            auto dp = to_latency(dp_t);
            if (dp < l_min) l_min = dp;
            if (dp > l_max) l_max = dp;
            l_sum += dp;
        }
        latency_t l_avg = l_sum / this->samples_sum;
        this->latency_avg = l_avg;
        this->latency_min = l_min;
        this->latency_max = l_max;
        this->latency_sum = l_sum;
        this->throughput_mbps = (download_sum * 1.0 / units::mib) / (duration.count() * 1.0 / units::ms_per_sec);
    }
    // --------------------------------------------------------------------------------
    std::string ByteRange::as_http_header() const {
        std::ostringstream s;
        s << "bytes=" << first_byte << "-" << last_byte;
        return s.str();
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::s3
