//
// Created by Maximilian Kuschewski on 2021-03-18
//
#include "benchmark/s3/S3Benchmark.hpp"
#include "benchmark/IO.hpp"

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <thread>
// #include <liburing.h>


namespace benchmark::s3 {
    // ----------------------------------------------------------------------------------------------------
    template<>
    RunResults S3Benchmark<URING>::do_run(RunParameters &params) {
        auto max_obj_size = this->fetch_object_size();
        std::vector<latency_t> results(params.sample_count * params.thread_count);
        std::vector<std::thread> threads;

        auto signed_uri =
            client.GeneratePresignedUrl(config.bucket_name, config.object_name,
                                        Aws::Http::HttpMethod::HTTP_GET);
        auto bucket_url = config.bucket_url();
        std::string newline = "\r\n";
        std::string protocol = "http://";
        auto shared_http_header =
            "GET " + signed_uri.substr(bucket_url.size() + protocol.size()) + " HTTP/1.1\r\n" +
            "Host: " + bucket_url + "\r\n" +
            "Range: ";
        auto max_range_header = ByteRange{ max_obj_size, max_obj_size }.as_http_header_line();
        auto max_header_alloc = shared_http_header.size() + max_range_header.size() + newline.size();

        // std::cout << "uri is " << signed_uri << std::endl <<
        //     "shared_header is " << shared_http_header << std::endl;
        // return RunResults { results , latency_t::zero() };

        clock::time_point start_time;
        std::atomic<bool> do_start = false;

        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
          threads.emplace_back([&, t_id]() {
            constexpr unsigned concurrent_requests = 64;
            constexpr unsigned recv_mask_set = 1u << ((sizeof(unsigned) << 3u) - 1);
            constexpr unsigned recv_mask = -1u ^ recv_mask_set;

            auto resp_field_size =  params.payload_size + 1024 * 16;
            std::array<int, concurrent_requests> fds;
            std::array<unsigned, concurrent_requests> req_size;
            std::vector<std::vector<char>> req(concurrent_requests);
            std::vector<std::vector<char>> res(concurrent_requests);
            for (unsigned i = 0; i != concurrent_requests; ++i) {
                req[i].reserve(max_header_alloc);
                auto length = shared_http_header.copy(req[i].data(), shared_http_header.size());
                auto range = this->random_range_in(params.payload_size, max_obj_size).as_http_header_line();
                length += range.copy(req[i].data() + length, range.size());
                length += newline.copy(req[i].data() + length, newline.size());
                req[i][length + 1] = '\0';
                req_size[i] = length;
                res[i].resize(resp_field_size);
            }

            std::string expected = "HTTP/1.1 206 Partial Content";
            auto received = 0u;
            auto in_queue = 0u;
            SocketIO io(concurrent_requests * 2); // TODO: *2 really required?
            auto addr_info = io.lookup(bucket_url.c_str(), 80);
            // wait until threads are ready
            if (t_id != params.thread_count - 1) {
              while (!do_start) {
              } // wait until all threads are started
            } else {
              do_start = true; // the last started thread sets the start time
              start_time = clock::now();
            }
            // send initial batch of requests
            for (;in_queue != concurrent_requests; ++in_queue) {
                auto [fd, _] = io.connect(&addr_info); // TODO: cache the lookup
                if (fd == -1){
                    throw std::runtime_error("Could not connect to S3 via socket.");
                }
                fds[in_queue] = fd;
                io.send_async<false>(fd, req[in_queue].data(), req_size[in_queue], in_queue);
            }
            io.submit_queue();
            // send rest of requests
            while (received < params.sample_count) {
                auto cdata = io.wait_completion();
                auto code = cdata.first;
                auto event = cdata.second;
                auto id = event->user_data;
                if (code != 0) {
                    std::cout << "Waiting returned " << code << "for id " << id << ", not doing anything..." << std::endl;
                    io.mark_seen(event);
                    continue;
                }
                io.mark_seen(event);
                if ((id | recv_mask) == -1u) { // mask set, receive event
                    id &= recv_mask;
                    // TODO
                    auto& memspace = res[id];
                    auto res = event->res;
                    memspace[params.payload_size] = '\0';
                    std::cout << "event res is " << res << std::endl;
                    // std::cout << "response is " << memspace.data() << std::endl;
                    // TODO check if complete content was submitted here,
                    // only submit new request when old one was really done
                    if (::strcmp(memspace.data(), expected.c_str()) != 0) {
                        memspace[expected.size()] = '\r';
                        std::cerr << "got " << memspace.data() << std::endl;
                        throw std::runtime_error("Unexpected format");
                    }
                    ++received;
                    io.close(fds[id]);
                    if (received < params.sample_count) {
                        fds[id] = io.connect(&addr_info).first; // TODO can sockets be reused? can lookup be cached?

                        if (fds[id] == -1) {
                          throw std::runtime_error(
                              "Could not connect to S3 via socket.");
                        }
                        io.send_async<true>(fds[id], req[id].data(),
                                            req_size[id], id);
                    }
                } else { // mask not set, 'done sending' event
                  // TODO sending this here assumes all responses come in after the write_request finishes. is this true?
                  // io.recv_async<true>(fds[id], res[id].data(), resp_field_size, id | recv_mask_set);
                  auto length = shared_http_header.copy(req[id].data(), shared_http_header.size());
                  auto range = this->random_range_in(params.payload_size, max_obj_size).as_http_header_line();
                  length += range.copy(req[id].data() + length, range.size());
                  length += newline.copy(req[id].data() + length, newline.size());
                  req[id][length + 1] = '\0';
                  req_size[id] = length;
                  // std::cout << "Sending " << req[id].data() << std::endl;
                }
            }
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
    // ----------------------------------------------------------------------------------------------------
}  // namespace benchmark::s3
