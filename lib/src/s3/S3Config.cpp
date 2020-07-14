//
// Created by maxi on 14.07.20.
//

#include "benchmark/s3/S3Config.hpp"

namespace benchmark::s3 {
    S3Config::S3Config(Config &&config)
            : Config(std::move(config))
            , client_config() {
        this->sanitize_client_config();
    }

    const Aws::Client::ClientConfiguration& S3Config::aws_config() const {
        return client_config;
    }

    Aws::String S3Config::region_name() const {
        return this->client_config.region;
    }

    void S3Config::sanitize_client_config() {
        this->client_config.region = region;
        this->client_config.maxConnections = std::abs(static_cast<long>(this->threads_max));
        this->client_config.scheme = Aws::Http::Scheme::HTTP;
        this->client_config.httpRequestTimeoutMs = 3 * units::ms_per_min;
        this->client_config.verifySSL = false;
        this->client_config.enableTcpKeepAlive = true;
        // this->client_config.tcpKeepAliveIntervalMs = 60 * units::ms_per_sec;
    }
}  // namespace benchmark::s3
