//
// Created by Maximilian Kuschewski on 2020-05-06
//

#ifndef _BENCHMARK_S3_BENCHMARK_HPP
#define _BENCHMARK_S3_BENCHMARK_HPP

#include <chrono>
#include <ratio>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>

#include "benchmark/Util.hpp"
#include "benchmark/Config.hpp"
#include "benchmark/Logger.hpp"

namespace benchmark::s3 {
    // --------------------------------------------------------------------------------
    using ObjectHead = Aws::S3::Model::HeadObjectOutcome;
    using latency_t = std::chrono::duration<size_t, std::chrono::milliseconds::period>;
    // --------------------------------------------------------------------------------
    struct ByteRange {
        size_t first_byte;
        size_t last_byte;
        inline size_t length() const { return last_byte - first_byte; }
        [[nodiscard]] std::string as_http_header() const;
    };
    // --------------------------------------------------------------------------------
    struct RunParameters {
        size_t sample_count;
        size_t thread_count;
        size_t payload_size;
    };
    // --------------------------------------------------------------------------------
    struct RunResults {
        std::vector<latency_t> data_points;
        latency_t overall_time;
    };
    // --------------------------------------------------------------------------------
    struct RunStats : RunParameters {
        double throughput_mbps;
        latency_t latency_avg;
        latency_t latency_sum;
        latency_t latency_min;
        latency_t latency_max;
        latency_t duration;
        size_t download_sum;
        size_t samples_sum;
        RunStats(const RunParameters &params, const RunResults &run);
    };
    // --------------------------------------------------------------------------------
    struct S3Logger : Logger, RunLogger<RunParameters, RunStats> {
        explicit S3Logger(Logger &logger) : Logger(logger) {}
        void print_run_footer() const override;
        void print_run_header() const override;
        void print_run_stats(const RunStats &stats) const override;
        void print_run_params(const RunParameters &params) const override;
    };
    // --------------------------------------------------------------------------------
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
    // --------------------------------------------------------------------------------
    class S3Benchmark {
        const S3Config &config;
        Aws::S3::S3Client client;
    public:
        explicit S3Benchmark(const S3Config &config);

        void list_buckets() const;
        [[nodiscard]] size_t fetch_object_size() const;
        [[nodiscard]] latency_t fetch_range(const ByteRange &range, char* outbuf, size_t bufsize) const;
        [[nodiscard]] latency_t fetch_object(const Aws::S3::Model::GetObjectRequest &req) const;

        [[nodiscard]] static ByteRange random_range_in(size_t size, size_t max_value) ;
        [[nodiscard]] RunResults do_run(RunParameters &params) const;
        void run_full_benchmark(S3Logger &logger) const;
    };
    // --------------------------------------------------------------------------------
}

#endif // _BENCHMARK_S3_BENCHMARK_HPP
