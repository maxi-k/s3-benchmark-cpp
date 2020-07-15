//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
#include <chrono>
#include <vector>
#include <algorithm>
#include "benchmark/Util.hpp"

namespace benchmark::ram {
    // --------------------------------------------------------------------------------
    RamBenchmark::RamBenchmark(const RamConfig &config)
        : config(config) { }
    // --------------------------------------------------------------------------------
    // From: https://codearcana.com/posts/2013/05/18/achieving-maximum-memory-bandwidth.html
    inline void write_memory_rep_stosq(void* buffer, size_t size, int to_write) {
        asm("cld\n"
            "rep stosq"
        : : "D" (buffer), "c" (size / sizeof(size_t)), "a" (to_write) );
    }
    // --------------------------------------------------------------------------------
    RunResults RamBenchmark::do_run(const RunParameters &params, std::vector<char> *memspace1, std::vector<char> *memspace2) const {
        auto output_size = params.sample_count * params.thread_count;
        auto word_size = sizeof(size_t);
        // [ ...thread1_samples, ...thread2_samples, ...]
        std::vector<duration_t> latencies(output_size);
        std::vector<duration_t> durations(output_size);

        clock::time_point start_time;
        Barrier barrier(params.thread_count + 1);
        std::vector<std::thread> threads;
        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
            threads.emplace_back([&, this, t_id]() {
                auto idx_start = params.sample_count * t_id;
                auto tspace1 = memspace1->data() + idx_start;
                auto tspace2 = memspace2->data() + idx_start;
                barrier.wait();
                for (int n_sample = 0; n_sample < params.sample_count; ++n_sample) {
                   auto sample_idx = idx_start + n_sample;
                   auto input = n_sample % 2 == 0 ? tspace1 : tspace2;
                   auto t_start = clock::now();
                   // write_memory_rep_stosq(input, params.payload_size, n_sample);
                   memset(input, n_sample, params.payload_size);
                   auto t_end = clock::now();
                   durations[sample_idx] = t_end - t_start;
                }
            });
        }
        start_time = clock::now();
        barrier.wait();

        for (auto &thread : threads) {
            thread.join();
        }
        clock::time_point end_time = clock::now();
        return RunResults{
            durations,
            std::chrono::duration_cast<time_resolution_t>(end_time - start_time)
        };
    }
    // --------------------------------------------------------------------------------
    void RamBenchmark::run_full_benchmark(RamLogger &logger) const {
        auto params = RunParameters{ config.samples, 1, 0,  };
        auto read_space1 = new std::vector<char>(config.payloads_max * config.threads_max);
        auto read_space2 = new std::vector<char>(config.payloads_max * config.threads_max);
        // TODO: consider config.payloads_step
        for (size_t payload_size = config.payloads_min; payload_size <= config.payloads_max; payload_size *= 2) {
            params.payload_size = payload_size;
            logger.print_run_params(params);
            logger.print_run_header();
            // TODO: consider config.threads_step
            for (size_t thread_count = config.threads_min; thread_count <= config.threads_max; thread_count *= 2) {
                params.thread_count =  thread_count;
                auto results = this->do_run(params, read_space1, read_space2);
                auto stats = RunStats(params, results);
                logger.print_run_stats(stats);
            }
            logger.print_run_footer();
        }
        delete read_space1;
        delete read_space2;
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram
