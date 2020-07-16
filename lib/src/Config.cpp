//
// Created by Maximilian Kuschewski on 07.06.20.
//
#include "benchmark/Config.hpp"
#include <aws/core/config/AWSProfileConfigLoader.h>
#include <string>
#include <algorithm>
#include <utility>

namespace benchmark {

    EC2Config::EC2Config() {
        // From https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/instancedata-data-retrieval.html
        std::string prefix = "http://169.254.169.254/latest/meta-data/";
        try {
            this->ec2_instance_id = http::curl_get(prefix + "instance-id");
            this->ec2_instance_type = http::curl_get(prefix + "instance-type");
            this->ec2_region = http::curl_get(prefix + "placement/availability-zone");
        } catch (std::runtime_error &e) {
            this->ec2_instance_id = "unknown-instance-id";
            this->ec2_instance_type = "unknown-instance-type";
            this->ec2_region = "unknown-region";
        }
        this->hw_thread_count = hardware::thread_count();
    }

    Config::Config(ConfigParameters &&parameters) : ConfigParameters(std::move(parameters)) {
        this->sanitize_params();
    }

    void Config::sanitize_params() {
        quiet = !dry_run && quiet;
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
}  // namespace benchmark
