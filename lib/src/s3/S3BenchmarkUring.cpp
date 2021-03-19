//
// Created by Maximilian Kuschewski on 2021-03-18
//
#include "benchmark/s3/S3Benchmark.hpp"
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <thread>

#include "benchmark/IO.hpp"
#include "benchmark/Util.hpp"


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
        std::string header_end_needle = "\r\n\r\n";
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
            constexpr unsigned recv_mask_set = 1u << ((sizeof(unsigned) << 3u) - 1);
            constexpr unsigned recv_mask = -1u ^ recv_mask_set;
            auto concurrent_requests = config.async_count;

            auto resp_field_size =  params.payload_size + 1024 * 128;
            std::vector<int> fds(concurrent_requests);
            std::vector<unsigned> res_header_size(concurrent_requests);
            std::vector<unsigned> res_size(concurrent_requests);
            std::vector<unsigned> req_size(concurrent_requests);
            std::vector<std::vector<char>> req(concurrent_requests);
            std::vector<std::vector<char>> res(concurrent_requests);
            for (unsigned i = 0; i != concurrent_requests; ++i) {
              // request bookkeping
              req[i].reserve(max_header_alloc);
              auto length = shared_http_header.copy(req[i].data(), shared_http_header.size());
              auto range = this->random_range_in(params.payload_size, max_obj_size).as_http_header_line();
              length += range.copy(req[i].data() + length, range.size());
              length += newline.copy(req[i].data() + length, newline.size());
              req[i][length + 1] = '\0';
              req_size[i] = length;
              // response bookkeeping
              res[i].resize(resp_field_size);
              res_size[i] = 0;
              res_header_size[i] = 0;
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
              if (fd == -1) {
                throw std::runtime_error("Could not connect to S3 via socket.");
              }
              fds[in_queue] = fd;
              io.send_async<false>(fd, req[in_queue].data(), req_size[in_queue], in_queue);
            }
            io.submit_queue();
            // send rest of requests
            while (received < params.sample_count) {
              auto [code, id] = io.wait_completion_result();
              if (code < 0) {
                std::cout << "Waiting returned " << code << "for id " << id
                          << ", not doing anything..." << std::endl;
                continue;
              }
              if ((id | recv_mask) == -1u) { // mask set, receive event
                id &= recv_mask;
                // TODO
                if (code > 0) { // received N bytes
                  auto &memspace = res[id];
                  if (res_size[id] == 0) { // first package -> should be http header
                    auto &header_line_end = memspace[expected.size()];
                    auto header_line_char = header_line_end;
                    header_line_end = '\0';
                    if (0 != ::strcmp(memspace.data(), expected.c_str())) { // but http header was not what was expected
                      memspace[expected.size()] = header_line_char;
                      memspace[params.payload_size] = '\0';
                      std::cerr << "for id " << id << ": got " << memspace.data() << std::endl;
                      throw std::runtime_error("Unexpected format: could not find HTTP header start.");
                    } else {
                        // std::cout << "Received first package for id " << id << " with size " << code << std::endl;
                        auto header_end = memspace.data();
                        auto found = predicate::find_next(&header_end, &(*memspace.end()), header_end_needle.data(), &(*header_end_needle.end()));
                        if (!found) {
                            throw std::runtime_error("Unexpected format: could not find HTTP header end.");
                        }
                        auto header_len = header_end - memspace.data();
                        res_header_size[id] = header_len;
                        // std::cout << "Found header length " << header_len << std::endl;
                    }
                  } else {
                    // memspace[params.payload_size] = '\0';
                    // std::cout << "Received follow-up package for id " << id << " with size " << code << std::endl;
                  }
                  res_size[id] += code; // count up received bytes
                } else if (res_size[id] < params.payload_size + res_header_size[id]) { // code == 0 -> last package
                  throw std::runtime_error("Got recv code 0 but not everything read from socket");
                }
                // received everything completely
                if (res_size[id] >= params.payload_size + res_header_size[id]) {
                  ++received;
                  res_size[id] = 0;
                  // io.close(fds[id]);
                  if (received < params.sample_count) {
                    // fds[id] = io.connect(&addr_info).first; // TODO can sockets be reused?
                    // if (fds[id] == -1) {
                    //   throw std::runtime_error("Could not connect to S3 via socket.");
                    // }
                    io.send_async<true>(fds[id], req[id].data(), req_size[id], id);
                  }
                } else {
                  io.recv_async<true>(fds[id], res[id].data() + res_size[id], resp_field_size - res_size[id], id | recv_mask_set);
                }
              } else { // mask not set, 'done sending' event
                // TODO sending this here assumes all responses come in after
                // the write_request finishes. is this true?
                io.recv_async<true>(fds[id], res[id].data(), resp_field_size, id | recv_mask_set);
                auto length = shared_http_header.copy(req[id].data(), shared_http_header.size());
                auto range = this->random_range_in(params.payload_size, max_obj_size).as_http_header_line();
                length += range.copy(req[id].data() + length, range.size());
                length += newline.copy(req[id].data() + length, newline.size());
                req[id][length + 1] = '\0';
                req_size[id] = length;
                // std::cout << "Sending " << req[id].data() << std::endl;
              }
            }
            for (auto& fd : fds) {
                io.close(fd);
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
