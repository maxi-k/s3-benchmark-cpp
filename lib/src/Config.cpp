//
// Created by Maximilian Kuschewski on 07.06.20.
//
#include "s3benchmark/Config.hpp"
#include <aws/core/config/AWSProfileConfigLoader.h>
#include <algorithm>
#include <utility>
#include <exception>

namespace s3benchmark {
    s3benchmark::Config::Config(ConfigParameters &&parameters)
        : client_config()
        , ConfigParameters(std::move(parameters)) {
        this->sanitize_params();
        this->sanitize_client_config();
    }

    const Aws::Client::ClientConfiguration& Config::aws_config() const {
        return client_config;
    }

    Aws::String Config::region_name() const {
        return this->client_config.region;
    }

    void Config::sanitize_params() {
        threads_min = std::min(threads_min, threads_max);
        payloads_min = std::min(payloads_min, payloads_max);
        if (payloads_step <= 1) {
            throw std::runtime_error("Payload size must increase in each step, please provide -payloads-step > 1!");
        }
        if (threads_step == 0 || threads_step == 1) {
            throw std::runtime_error("Threads step cannot be 0 or one. Provide negative values for additive behavior, "
                                     "positive for multiplicative. Use -dry-run to test behavior without running.");
        }
        if (full) {
            // if running the full exhaustive test, the threads and payload arguments get overridden with these
            threads_static = false;
            threads_min = 1;
            threads_max = 2;
            threads_step = -1;
            payloads_min = 1;    //   1 MB
            payloads_max = 256;  // 256 MB
        }

        if (throttling_mode) {
            // if running the network throttling test, the threads and payload arguments get overridden with these
            threads_static = true;
            threads_min = 2;
            threads_max = 2;
            threads_step = -1;
            payloads_min = 100;  // 100 MB
            payloads_max = 100;  // 100 MB
        }
    }

    void Config::sanitize_client_config() {
        this->client_config.region = region;
        this->client_config.httpRequestTimeoutMs = 3 * units::ms_per_min;
    }

}  // namespace s3benchmark
