//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <thread>

#include "s3benchmark/Config.hpp"
#include "s3benchmark/Types.hpp"
#include "s3benchmark/HttpClient.hpp"

namespace s3benchmark {

    Benchmark::Benchmark(const Config &config)
            : config(config)
            , client(Aws::S3::S3Client(config.aws_config()))
            , presigned_url(this->client.GeneratePresignedUrl(config.bucket_name, config.object_name, Aws::Http::HttpMethod::HTTP_GET, URL_TIMEOUT_S)) {
        // this->presigned_url.replace(0, 5, "http"); // https -> http
        std::cout << "Presigned URL " << presigned_url << std::endl;
        // throw std::runtime_error("break");
    }

    void Benchmark::list_buckets() const {
        auto resp = client.ListBuckets();
        for (auto& bucket : resp.GetResult().GetBuckets()) {
            std::cout << "Found bucket: " << bucket.GetName() << std::endl;
        }
    }

    size_t Benchmark::fetch_object_size() const {
        auto req = Aws::S3::Model::HeadObjectRequest()
                .WithBucket(config.bucket_name)
                .WithKey(config.object_name);
        auto resp = client.HeadObject(req);
        if (!resp.IsSuccess()) {
            throw std::runtime_error("Could not fetch object head.");
        }
        auto len = resp.GetResult().GetContentLength();
        return len;
    }

    latency_t Benchmark::fetch_range(const std::shared_ptr<Aws::Http::HttpClient> c, const ByteRange &range, char* outbuf, size_t bufsize) const {
        // Put data into outbuf
        auto req = Aws::Http::CreateHttpRequest(this->presigned_url, Aws::Http::HttpMethod::HTTP_GET, [&outbuf, &bufsize]() {
            // Faster method than stringstream?
            // auto stream = Aws::New<Aws::StringStream>("S3Client");
            // stream->rdbuf()->pubsetbuf(outbuf, bufsize);
            // return stream;
            return Aws::New<Aws::FStream>("S3Client", "/dev/null", std::ios_base::out);
        });
        req->SetHeaderValue("Range", range.as_http_header());
        auto start = clock::now();
        // TODO: Test using aws http range
        auto res = c->MakeRequest(req);
        if (res->HasClientError()) {
            std::cout << res->GetClientErrorMessage() << std::endl;
        }
        auto end = clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    ByteRange Benchmark::random_range_in(size_t size, size_t max_value) {
        if (size > max_value) {
            throw std::runtime_error("Cannot create byte range larger than max size.");
        }
        auto offset = random::in_range<size_t>(0, max_value - size);
        return { offset, offset + size };
    }

    size_t Benchmark::fetch_url_curl_callback(char *body, size_t size_mult, size_t nmemb, void *userdata) {
        auto size = size_mult * nmemb;
        auto output = static_cast<char*>(userdata);
        // payload size is always the same, just copy memcpy
        // TODO: consider just forgoing this step, not relevant for benchmark.
        // memcpy(output, body, size);
        return size;
    }

    void Benchmark::fetch_url_curl(CURL *curl, ByteRange &range, latency_t *latency_output, char* content_output) const {
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_RANGE, range.as_string().c_str());
        auto start = clock::now();
        res = curl_easy_perform(curl);
        auto end = clock::now(); // TODO: is this after whole body arrives or after header arrives? -> curl doc
        if (res != 0) {
            throw std::runtime_error("Failed fetching bucket result");
        }
        *latency_output = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    RunResults Benchmark::do_run(RunParameters &params) const {
        auto overall_sample_count = params.sample_count * params.thread_count;
        // Pre-generate request ranges
        std::vector<ByteRange> request_ranges;
        request_ranges.reserve(overall_sample_count);
        for (int i = 0; i < overall_sample_count; ++i) {
            request_ranges.push_back(random_range_in(params.payload_size, params.content_size));
        }
        // Thread variables for measuring time
        clock::time_point start_time;
        bool do_start = false;
        // Allocate memory for the results
        size_t http_response_size = params.payload_size + (20ul << 10ul); // + http header est. 20kb
        std::vector<char> outbuf(params.thread_count * http_response_size);
        std::vector<latency_t> results(overall_sample_count);

        // Create a Shared header string
        // TODO: dynamic substring after hostname
        auto shared_http_header = "GET " + this->presigned_url.substr(54) + " HTTP/1.1\r\n" +
                "Host: " + this->config.bucket_name + ".s3.eu-central-1.amazonaws.com\r\n" +
                "Connection: keep-alive\r\n" +
                "Range: ";
        auto shared_header_length = shared_http_header.length();
        auto max_strlen_range = ByteRange{params.content_size, params.content_size};
        auto base_http_header = shared_http_header + max_strlen_range.as_http_header() + "\r\n\r\n";
        auto host_def = HttpClient::lookup_host(this->config.bucket_name + ".s3.amazonaws.com");

        // Create a list for the threads, start them
        std::vector<std::thread> threads;
        do_start = true; // the last started thread sets the start time
        start_time = clock::now();
        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
           threads.emplace_back([this, t_id, &base_http_header, &shared_header_length, &host_def, &http_response_size, &outbuf, &request_ranges, &params, &results, &do_start, &start_time]() {
               auto buf = outbuf.data() + t_id * http_response_size;
               auto idx_start = params.sample_count * t_id;
               unsigned bytes_recv = 0;
               size_t cnt = 0;
               auto noop_callback = [&cnt, &bytes_recv, &t_id](size_t recv_length, char* buf){
                   // std::cout << "Received " << recv_length << " bytes of data on thread " << t_id << std::endl;
                   bytes_recv += recv_length;
                   ++cnt;
               };
               auto http_client = HttpClient(base_http_header, shared_header_length, noop_callback);
               http_client.open_connection(host_def);

               // if (t_id != params.thread_count - 1) {
               //     while (!do_start) { } // wait until all threads are started
               // } else {
               //     do_start = true; // the last started thread sets the start time
               //     start_time = clock::now();
               // }
               auto dyn_length = http_client.dynamic_header_size();
               for (unsigned i = 0; i < params.sample_count; ++i) {
                   auto range_str = request_ranges[idx_start + i].as_http_header() + "\r\n\n\n\n\n\n\n";
                   memcpy(http_client.dynamic_header(), range_str.c_str(), dyn_length);
                   // TODO: fix start/end time for duration
                   auto t_start = clock::now();
                   http_client.execute_request(http_response_size, buf); // payload + header
                   auto t_end = clock::now();
                   results[idx_start + i] = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
               }
               std::cout << "received bytes for thread" << t_id << " is " << bytes_recv << "\t\t in chunks: \t" << cnt << std::endl;
           });
        }

        for (auto &thread : threads) {
            thread.join();
        }
        clock::time_point end_time = clock::now();
        return RunResults{
            results,
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time)
        };
    }

    void Benchmark::run_full_benchmark(Logger &logger) const {
        // TODO: consider config.payloads_step
        auto params = RunParameters{ config.samples, 1, 0, this->fetch_object_size() };
        for (size_t payload_size = config.payloads_min; payload_size <= config.payloads_max; payload_size *= 2) {
            params.payload_size = payload_size;
            logger.print_run_params(params);
            logger.print_run_header();
            // TODO: consider config.threads_step
            for (size_t thread_count = config.threads_min; thread_count <= config.threads_max; thread_count *= 2) {
                params.thread_count =  thread_count;
                auto results = this->do_run(params);
                auto stats = RunStats(params, results);
                logger.print_run_stats(stats);
            }
            logger.print_run_footer();
        }
        // TODO: csv upload
    }
}  // namespace s3benchmark
