//
// Created by Maximilian Kuschewski on 2020-05-06
//
#include "s3benchmark/Benchmark.hpp"
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <sys/stat.h>
#include <sys/select.h>
#include <cerrno>

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
            , client(config.aws_config())
            , presigned_url(this->client.GeneratePresignedUrl(config.bucket_name, config.object_name, Aws::Http::HttpMethod::HTTP_GET, URL_TIMEOUT_S)) {
        // this->presigned_url.replace(0, 5, "http"); // https -> http
        std::cout << "Presigned URL " << presigned_url << std::endl;
        // throw std::runtime_error("break");
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

    ByteRange Benchmark::random_range_in(size_t size, size_t max_value) {
        if (size > max_value) {
            throw std::runtime_error("Cannot create byte range larger than max size.");
        }
        auto offset = random::in_range<size_t>(0, max_value - size);
        return { offset, offset + size };
    }

    TestEnv Benchmark::prepare_run(const RunParameters &params) const {
        auto overall_sample_count = params.sample_count * params.thread_count;
        // host definition
        auto host_def = HttpClient::lookup_host(this->config.bucket_name + ".s3.amazonaws.com");
        // requests
        // TODO: make more dynamic (substring, hostname)
        auto shared_http_header = "GET " + this->presigned_url.substr(54) + " HTTP/1.1\r\n" +
                                  "Host:  masters-thesis-mk.s3.eu-central-1.amazonaws.com\r\n" +
                                  "Range: ";
        auto shared_header_length = shared_http_header.length();
        std::vector<std::string> requests;
        requests.reserve(overall_sample_count);
        for (int i = 0; i < overall_sample_count; ++i) {
            auto range = random_range_in(params.payload_size, params.content_size);
            requests.push_back(shared_http_header + range.as_http_header() + "\r\n\r\n");
        }
        // connections
        std::vector<Connection> connections;
        connections.reserve(params.thread_count); // TODO: does socket_count == thread_count make sense?
        for (int i = 0; i < params.thread_count; ++i) {
            connections.push_back(HttpClient::create_connection(host_def, i));
        }
        // Prepare POSIX file descriptor sets
        fd_set send_set = Connection::make_fd_set(connections);
        fd_set recv_set = Connection::make_fd_set(connections);

        return TestEnv{
            connections,
            requests,
            host_def,
            static_cast<int>(connections.size()),
            send_set,
            recv_set,
            params.sample_count * params.thread_count,
            0,
            0,
            0
        };

    }

    void thread_count_http_responses(TestEnv &env, std::vector<char> &buffer, int &thread_stage) {
        while (thread_stage == 0);
        size_t counted_bytes = 0;
        while (env.received_responses < env.overall_sample_count) {
            while (counted_bytes >= env.received_bytes && thread_stage == 1);
            if (thread_stage != 1) { break; }
            auto &&[ skip, matches ] = http::skim_http_data(buffer.data() + counted_bytes, env.received_bytes - counted_bytes);
            if (matches > 0) {
                std::cout << "Found " << matches << "  matches, skipping content-length of " << skip << std::endl;
            } else {
                std::cout << "No matches";
            }
            counted_bytes += env.received_bytes + skip;
            env.received_responses += matches;
        }
    }

    struct SocketPool {
        std::vector<Connection> connections;
        std::mutex connections_lock;
        size_t closed_sockets;

        ~SocketPool() {
            for (auto &conn : connections) {
                conn.close_connection();
            }
            closed_sockets = connections.size();
        }
    };

    void thread_prepare_socket_pools(const Config &config, const RunParameters &params, TestEnv &env, SocketPool &pool, int &thread_stage) {
        while (thread_stage == 0);  // TODO: spin lock
        while (thread_stage == 1) {
            while(pool.closed_sockets == 0 && thread_stage == 1);  // TODO: spin lock
            if (thread_stage != 1) { break; }
            auto host = HttpClient::lookup_host(config.bucket_name + ".s3.amazonaws.com");
            // std::cout << "Looked up host, creating new socket pool" << std::endl;
            auto reopened_sockets = 0;
            for (auto &conn : pool.connections) {
                if (!conn.is_open()) {
                    pool.connections_lock.lock();
                    conn = HttpClient::create_connection(host, conn.id);
                    ++reopened_sockets;
                    pool.connections_lock.unlock();
                }
            }
            while (reopened_sockets > 0) {
                --reopened_sockets;
                --pool.closed_sockets;
            }
        }
    }

    RunResults Benchmark::do_run(const RunParameters &params) const {
        auto env = this->prepare_run(params);
        // Allocate memory for the results
        size_t http_response_size = params.payload_size + (1ul << 10ul); // + http header est. 1kb
        std::vector<char> outbuf(params.thread_count * http_response_size);
        std::vector<latency_t> latencies; latencies.reserve(env.overall_sample_count * 10);
        std::vector<size_t> chunk_counts; chunk_counts.reserve(env.overall_sample_count * 10);
        std::vector<size_t> payload_sizes;  payload_sizes.reserve(env.overall_sample_count * 10);
        // Initialize the 'chunk received' handler
        const static char response_start[] = "HTTP/1.1 206 Partial Content";
        const static char http_header_end[] = "\r\n\r\n";
        HttpClient::response_handler_t handler([&](Connection& conn, size_t recv_size, char* buf) {
            env.received_bytes += recv_size;
            if (predicate::starts_with(buf, response_start)) { // TODO: how expensive is this? load onto extra thread?
                ++env.received_responses;
                auto nbuf = buf;
                auto content_start = predicate::find_next(&nbuf, buf + recv_size, http_header_end, http_header_end + sizeof(http_header_end) - 1);
                conn.pending_bytes -= (buf + recv_size) - nbuf - 1;
            } else {
                conn.pending_bytes -= recv_size;
            }
            if (conn.pending_bytes < 0) {
                std::cerr << "error during byte length calculation: " << conn.pending_bytes << std::endl;
            }
            // std::cout << "Received response:" << std::endl;
            // std::cout << "------------------------------------------------------------------------------" << std::endl;
            // std::cout << std::string(buf, recv_size) << std::endl;
            // std::cout << "------------------------------------------------------------------------------" << std::endl;
        });
        // Socket pools
        auto pool = SocketPool{
                env.connections,
                std::mutex(),
                0
        };
        fd_set recv_set;
        fd_set send_set;

        // Threads
        auto thread_stage = 0;
        // auto scan_thread = std::thread([&]() { thread_count_http_responses(env, outbuf, thread_stage); });
        auto conn_thread = std::thread([&]() { thread_prepare_socket_pools(config, params, env, pool, thread_stage); });
        // Add timing variables, start test
        clock::time_point start_time = clock::now();
        thread_stage = 1;
        while (env.received_responses < env.overall_sample_count) {
            struct timeval timeout_def{  // TODO: read from config
                10,
                500
            };
            while (pool.closed_sockets == params.thread_count) {
                std::cout << "Waiting for fresh sockets..." << std::endl;
            }
            pool.connections_lock.lock();
            recv_set = Connection::make_fd_set(pool.connections);
            send_set = Connection::make_fd_set(pool.connections); // TODO memcpy
            pool.connections_lock.unlock();
            auto select_res = select(env.set_count + 1, &recv_set, &send_set, nullptr, &timeout_def);

            if (select_res < 0) {
                auto err = errno;
                std::cerr << "An error occured while polling on the sockets: " << err << std::endl;
                // TODO: better error handling
                throw std::runtime_error("select(2) error.");
            }
            if (select_res == 0) {
                std::cerr << "Timeout on polling;" << std::endl;
                break;
            }
            for (auto &conn : pool.connections) {
                // Check available write sockets
                if(FD_ISSET(conn.socket, &send_set) && conn.pending_bytes == 0 && env.executed_requests < env.overall_sample_count) {
                    HttpClient::send_msg(conn, env.requests[env.executed_requests]);
                    FD_CLR(conn.socket, &send_set);
                    ++env.executed_requests;
                    ++conn.writes;
                    conn.pending_bytes += params.payload_size;
                }
                // Check available read sockets
                if (FD_ISSET(conn.socket, &recv_set) && conn.pending_bytes != 0 && conn.writes != 0 && env.received_responses < env.overall_sample_count) {
                    try {
                        auto read_stats = HttpClient::receive_msg(conn, http_response_size, outbuf.data() + http_response_size * conn.id, handler);
                        FD_CLR(conn.socket, &recv_set);
                        latencies.emplace_back(read_stats.read_duration);
                        chunk_counts.emplace_back(read_stats.chunk_count);
                        payload_sizes.emplace_back(read_stats.payload_size);
                    } catch (std::runtime_error &e) {
                        std::cerr << "Error while receiving message on socket" << conn.id << ": " << e.what() << std::endl;
                    }
                    if (conn.pending_bytes == 0) {
                        conn.close_connection();
                        ++pool.closed_sockets;
                    }
                }
            }
        }
        clock::time_point end_time = clock::now();

        thread_stage = 2;
        conn_thread.join();
        // scan_thread.join();

        return RunResults{
            latencies,
            payload_sizes,
            chunk_counts,
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
