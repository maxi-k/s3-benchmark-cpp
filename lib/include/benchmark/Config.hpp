//
// Created by Maximilian Kuschewski on 07.06.20.
//
#ifndef _BENCHMARK_CONFIG_HPP
#define _BENCHMARK_CONFIG_HPP

#include "Util.hpp"
#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>

namespace benchmark {
    using config_t = Aws::Client::ClientConfiguration;

    enum BenchType { S3, CPU, RAM, SSD };
    inline const char* BENCH_TYPE_NAMES[] = { "s3", "cpu", "ram", "ssd" };

    enum RamTestMode { READ, WRITE, READ_AVX };
    inline const char* RAM_MODE_NAMES[] = { "read", "write", "read-avx" };

    struct ConfigParameters {
        BenchType bench_type;
        bool dry_run;
        bool quiet;
        bool threads_static;
        double threads_min;
        double threads_max;
        double threads_step;
        size_t payloads_min;
        size_t payloads_max;
        size_t payloads_step;
        bool payloads_reverse;
        size_t samples;
        size_t samples_cap;
        std::string bucket_name;
        std::string object_name;
        std::string region;
        bool full;
        bool throttling_mode;
        std::string upload_csv;
        std::string upload_stats;
        RamTestMode ram_mode;
    };

    struct EC2Config {
        std::string ec2_instance_id;
        std::string ec2_instance_type;
        std::string ec2_region;
        size_t hw_thread_count;
        EC2Config();
    };

    class Config : public ConfigParameters, public EC2Config {
        void sanitize_params();
    public:
        explicit Config(ConfigParameters &&config);
    };
}

#endif //_BENCHMARK_CONFIG_HPP
