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
        auto host_lookup_time = clock::now();
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
            host_lookup_time,
            static_cast<int>(connections.size()),
            send_set,
            recv_set,
            0,
            0
        };

    }

    RunResults Benchmark::do_run(const RunParameters &params) const {
        auto overall_sample_count = params.sample_count * params.thread_count;
        auto env = this->prepare_run(params);
        // Allocate memory for the results
        size_t http_response_size = params.payload_size + (1ul << 10ul); // + http header est. 1kb
        std::vector<char> outbuf(overall_sample_count * http_response_size);
        std::vector<latency_t> latencies;
        std::vector<size_t> chunk_counts(env.connections.size(), 0); // TODO: per query chunk counts instead of per socket
        std::vector<size_t> payload_sizes(env.connections.size(), 0); // TODO: per query chunk sizes instead of per socket
        // Initialize the 'chunk received' handler
        std::function<void(size_t, char*)> handler([&outbuf, &env](size_t recv_size, char* buf) {
            // TODO: need to search the whole response for the search string; very slow. is there another way?
            // (1) Idea: find Content-Length field in header and then skip that many bytes
            // TODO: store next_pos in map { conn -> pos } and pass it as start_pos arg
            auto&& [next_pos, occ] = http::skim_http_data(buf, recv_size - 1, 0);
            if (occ > 0) {
                if (occ > 1) {
                    std::cout << "More than one occurance of the header in this package!" << std::endl;
                    std::cout << std::string(buf, recv_size) << std::endl;
                }
                while (occ != 0) {
                    ++env.received_responses;
                    --occ;
                }
                // if (recv_size >= new_sample_start.size() && predicate::starts_with(new_sample_start.c_str(), buf)) {
                // std::cout << "Received another " << occ << " responses, " << overall_sample_count - env.received_responses << " to go." << std::endl;
            }
        });
        auto buf_pos = 0;
        struct task_list_t {
            size_t recv_size;
            char* buffer;
            task_list_t* next;
        };
        std::mutex task_lock;
        task_list_t* task_list = nullptr;
        HttpClient::response_handler_t async_handler([&](const Connection& conn, size_t recv_size, char* buf) {
            buf_pos += recv_size;
            task_lock.lock();
            task_list = new task_list_t{ recv_size, buf, task_list };
            task_lock.unlock();
        });

        auto thread_stage = 0;
        auto worker_thread = std::thread([&]() {
            // TODO: spin locks
            while (thread_stage == 0);
            while (thread_stage == 1) {
                while (task_list == nullptr && thread_stage == 1);
                if (thread_stage != 1) { break; }
                task_lock.lock();
                if (task_list == nullptr ) {
                    task_lock.unlock();
                    break;
                }
                auto next_task = *task_list;
                task_list = next_task.next;
                task_lock.unlock();
                handler(next_task.recv_size, next_task.buffer);
            }
        });
        // Bookkeeping variables
        fd_set send_set;
        fd_set recv_set;
        // Add timing variables, start test
        clock::time_point start_time = clock::now();
        ++thread_stage;
        while (env.executed_requests < overall_sample_count || env.received_responses < overall_sample_count) {
            struct timeval timeout_def{  // TODO: read from config
                10,
                500
            };
            std::memcpy(&send_set, &env.send_set_all, sizeof(send_set));
            std::memcpy(&recv_set, &env.recv_set_all, sizeof(recv_set));
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
            for (auto &conn : env.connections) {
                // Check available read sockets
                if (FD_ISSET(conn.socket, &recv_set) && env.received_responses < overall_sample_count) {
                    try {
                        auto read_stats = HttpClient::receive_msg(conn, http_response_size, outbuf.data() + buf_pos, async_handler);
                        FD_CLR(conn.socket, &recv_set);
                        latencies.emplace_back(read_stats.read_duration);
                        chunk_counts.emplace_back(read_stats.chunk_count);
                        payload_sizes.emplace_back(read_stats.payload_size);
                    } catch (std::runtime_error &e) {
                        std::cerr << "Error while receiving message from the buffer, restarting socket..." << std::endl;
                        conn.close_connection();
                        // TODO: see what value makes sense here
                        if (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - env.last_host_lookup).count() > 30) {
                            env.last_host_lookup = clock::now();
                            env.remote_host = HttpClient::lookup_host(this->config.bucket_name + ".s3.amazonaws.com");
                        }
                        conn = HttpClient::create_connection(env.remote_host, conn.id);
                        --env.executed_requests;
                        // throw e;
                        // goto run_end;
                    }
                }
                // Check available write sockets
                if(FD_ISSET(conn.socket, &send_set) && env.executed_requests < overall_sample_count) {
                    HttpClient::send_msg(conn, env.requests[env.executed_requests]);
                    FD_CLR(conn.socket, &send_set);
                    ++env.executed_requests;
                    // if (env.executed_requests == overall_sample_count) {
                    //     std::cout << "Sent all requests!" << std::endl;
                    // }
                }
            }
        }
        run_end:

        ++thread_stage;
        worker_thread.join();

        clock::time_point end_time = clock::now();
        for (auto &conn : env.connections) {
            conn.close_connection();
        }

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
