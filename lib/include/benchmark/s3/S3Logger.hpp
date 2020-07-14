//
// Created by maxi on 14.07.20.
//

#ifndef _BENCHMARK_S3_LOGGER_H
#define _BENCHMARK_S3_LOGGER_H

#include "benchmark/Logger.hpp"

namespace benchmark::s3 {
    class S3Logger : public Logger {
    public:
        explicit S3Logger(std::ostream &output) : Logger(output) {}
        explicit S3Logger(Logger &logger) : Logger(logger) {}
        void print_run_header() const;
        void print_run_footer() const;
        void print_run_stats(const s3::RunStats &stats) const;
        void print_run_params(const s3::RunParameters &params) const;
    };
    // --------------------------------------------------------------------------------
}  // namespace benchmark::s3

#endif //S3BENCHMARK_S3_LOGGER_H
