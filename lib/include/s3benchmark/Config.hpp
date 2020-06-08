//
// Created by Maximilian Kuschewski on 07.06.20.
//
#ifndef _S3BENCHMARK_CONFIG_HPP
#define _S3BENCHMARK_CONFIG_HPP

#include "Util.hpp"
#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>

namespace s3benchmark {
    using config_t = Aws::Client::ClientConfiguration;

    struct ConfigParameters {
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
    };

    class Config : public ConfigParameters {
        Aws::Client::ClientConfiguration client_config;

        void sanitize_params();
        void sanitize_client_config();

    public:
        inline const static char* DEFAULT_REGION = "eu-central-1";
        inline const static char* DEFAULT_BUCKET_NAME = "masters-thesis-mk";
        inline const static char* DEFAULT_OBJECT_NAME = "benchmark/largefile.bin";

        explicit Config(ConfigParameters &&config);

        const Aws::Client::ClientConfiguration& aws_config() const;
        Aws::String region_name() const;
    };
}

#endif //_S3BENCHMARK_CONFIG_HPP
