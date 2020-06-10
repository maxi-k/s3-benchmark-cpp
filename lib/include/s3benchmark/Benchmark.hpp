//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _S3BENCHMARK_BENCHMARK_HPP
#define _S3BENCHMARK_BENCHMARK_HPP

#include <chrono>
#include <ratio>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include "Config.hpp"
#include "Logger.hpp"

namespace s3benchmark {
    using ObjectHead = Aws::S3::Model::HeadObjectOutcome;

    class Benchmark {
        using clock = std::chrono::steady_clock;

        const Config &config;
        Aws::S3::S3Client client;

        static const size_t URL_TIMEOUT_S = 1000;
        static const size_t CURL_TIMEOUT_MS = 18000;
        Aws::String presigned_url;

        static size_t fetch_url_curl_callback(char *body, size_t size_mult, size_t nmemb, void *userdata);
        void fetch_url_curl(CURL *curl, ByteRange &range, latency_t* latency_output, char* content_output) const;

    public:
        explicit Benchmark(const Config &config);

        void list_buckets() const;
        [[nodiscard]] size_t fetch_object_size() const;
        [[nodiscard]] latency_t fetch_range(const ByteRange &range, char* outbuf, size_t bufsize) const;

        [[nodiscard]] static ByteRange random_range_in(size_t size, size_t max_value) ;
        [[nodiscard]] RunResults do_run(RunParameters &params) const;
        void run_full_benchmark(Logger &logger) const;
    };
}

#endif // _S3BENCHMARK_BENCHMARK_HPP
