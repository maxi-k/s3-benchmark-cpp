//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
#include <chrono>
#include <vector>
#include "benchmark/Util.hpp"

namespace benchmark::ram {
    // --------------------------------------------------------------------------------
    RamBenchmark::RamBenchmark(const RamConfig &config)
        : config(config) { }
    // --------------------------------------------------------------------------------
    RunResults RamBenchmark::do_run(const RunParameters &params) const {
        auto output_size = params.sample_count * params.thread_count;
        auto file_size = params.thread_count * params.payload_size;
        // [ ...thread1_samples, ...thread2_samples, ...]
        std::vector<duration_t> latencies(output_size);
        std::vector<duration_t> durations(output_size);

        // TODO: create file for reading, fill it with something

        clock::time_point start_time;
        bool do_start = false;

        std::vector<std::thread> threads;
        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
            threads.emplace_back([&, this, t_id]() {
                auto idx_start = params.sample_count * t_id;
                if (t_id != params.thread_count - 1) {
                    while (!do_start) { }  // wait until all threads are started
                } else {
                    do_start = true;  // the last started thread sets the start time
                    start_time = clock::now();
                }
                // TODO: read file
            });
        }

        for (auto &thread : threads) {
            thread.join();
        }
        clock::time_point end_time = clock::now();
        return RunResults{
            latencies,
            durations,
            std::chrono::duration_cast<time_resolution_t>(end_time - start_time)
        };
    }
    // --------------------------------------------------------------------------------
    void RamBenchmark::run_full_benchmark(RamLogger &logger) const {
        // TODO: consider config.payloads_step
        auto params = RunParameters{ config.samples, 1, 0 };
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
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram
