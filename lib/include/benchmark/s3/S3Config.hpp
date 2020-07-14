//
// Created by maxi on 14.07.20.
//

#ifndef _BENCHMARK_S3_CONFIG_H
#define _BENCHMARK_S3_CONFIG_H

#include "benchmark/Config.hpp"

namespace benchmark::s3 {
    class S3Config : public Config {
        Aws::Client::ClientConfiguration client_config;
        void sanitize_client_config();

    public:
        inline const static char* DEFAULT_BUCKET_NAME = "masters-thesis-mk";
        inline const static char* DEFAULT_OBJECT_NAME = "benchmark/largefile.bin";
        inline const static char* DEFAULT_REGION = "eu-central-1";

        explicit S3Config(Config &&config);

        [[nodiscard]] Aws::String region_name() const;
        [[nodiscard]] const Aws::Client::ClientConfiguration& aws_config() const;
    };
}  // benchmark::s3

#endif //_BENCHMARK_S3_CONFIG_H
