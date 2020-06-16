//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP

#include <chrono>
#include <ratio>
#include <aws/core/Aws.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/client/AWSClient.h>
#include <aws/s3/S3Client.h>
#include "Config.hpp"
#include "Logger.hpp"

namespace s3benchmark {
    using ObjectHead = Aws::S3::Model::HeadObjectOutcome;

    class Benchmark {

        const Config &config;
        Aws::S3::S3Client client;

        static const size_t URL_TIMEOUT_S = 1000;
        static const size_t CURL_TIMEOUT_S = 3 * 60;
        Aws::String presigned_url;

        TestEnv prepare_run(const RunParameters &params) const;

    public:
        explicit Benchmark(const Config &config);

        [[nodiscard]] size_t fetch_object_size() const;

        [[nodiscard]] static ByteRange random_range_in(size_t size, size_t max_value) ;
        [[nodiscard]] RunResults do_run(const RunParameters &params) const;
        void run_full_benchmark(Logger &logger) const;
    };
}

#endif // _S3BENCHMARK_BENCHMARK_HPP
