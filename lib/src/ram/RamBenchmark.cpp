//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
// AVX
#include <emmintrin.h>
#include <pmmintrin.h>
#include <immintrin.h>
#include <nmmintrin.h>

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
    inline void write_memory_rep_stosq(void* buffer, size_t n_words, int to_write) {
        asm("cld\n"
            "rep stosq"
        : : "D" (buffer), "c" (n_words), "a" (to_write) );
    }
    // --------------------------------------------------------------------------------
    void RamBenchmark::thread_task_write(const RunParameters &params, ThreadTaskParams &t) {
        auto read_sum = 0ul;
        t.barrier.wait();
        for (size_t n_sample = 0; n_sample < params.sample_count; ++n_sample) {
            auto input = n_sample % 2 == 0 ? t.tspace1 : t.tspace2;
            auto t_start = clock::now();
            // write_memory_rep_stosq(input, params.payload_words, n_sample);
            memset(input, n_sample, params.payload_words * sizeof(size_t));
            auto t_end = clock::now();
            read_sum += params.payload_words * sizeof(size_t);
            t.durations[n_sample] = t_end - t_start;
        }
        *t.read_count = read_sum;
    }
    // --------------------------------------------------------------------------------
    void RamBenchmark::thread_task_read(const RunParameters &params, ThreadTaskParams &t) {
        auto read_sum = 0ul;
        auto res = 0ul;
        t.barrier.wait();
        for (size_t n_sample = 0; n_sample < params.sample_count; ++n_sample) {
            auto input = n_sample % 2 == 0 ? t.tspace1 : t.tspace2;
            auto t_start = clock::now();
            for (size_t i = 0; i != params.payload_words; ++i) {
                res += input[i];
            }
            auto t_end = clock::now();
            read_sum += params.payload_words * sizeof(size_t);
            t.durations[n_sample] = t_end - t_start;
        }
        *t.result = res;
        *t.read_count = read_sum;
    }
    // --------------------------------------------------------------------------------
    void RamBenchmark::thread_task_read_avx(const RunParameters &params, ThreadTaskParams &t) {
        auto read_sum = 0ul;
        __m256i res = _mm256_loadu_si256(reinterpret_cast<__m256i*>(t.tspace1));
        t.barrier.wait();
        for (size_t n_sample = 0; n_sample < params.sample_count; ++n_sample) {
            auto input = n_sample % 2 == 0 ? t.tspace1 : t.tspace2;
            auto t_start = clock::now();
            auto words_per_vector = sizeof(__m256i) / sizeof(size_t);
            for (size_t i = 0; i != params.payload_words; i += words_per_vector) {
                auto block = _mm256_loadu_si256(reinterpret_cast<__m256i*>(input + i));
                res = _mm256_add_epi64(res, block);
            }
            auto t_end = clock::now();
            read_sum += params.payload_words * sizeof(size_t);
            t.durations[n_sample] = t_end - t_start;
        }
        *t.result = res[0];
        *t.read_count = read_sum;
    }
    // --------------------------------------------------------------------------------
    RunResults RamBenchmark::do_run(const RunParameters &params, size_t* memspace1, size_t* memspace2) const {
        // [ ...thread1_samples, ...thread2_samples, ...]
        std::vector<duration_t> durations(params.sample_count * params.thread_count);
        std::vector<size_t> read_counts(params.thread_count);
        std::vector<size_t> results(params.thread_count);

        Barrier barrier(params.thread_count + 1);
        std::vector<ThreadTaskParams> thread_params;
        std::vector<std::thread> threads;
        auto fn = thread_task_map[config.ram_mode];
        for (unsigned t_id = 0; t_id != params.thread_count; ++t_id) {
            auto mem_idx = config.payloads_max * t_id / sizeof(size_t);
            thread_params.emplace_back(ThreadTaskParams{
                    t_id,
                    barrier,
                    memspace1 + mem_idx,
                    memspace2 + mem_idx,
                    durations.data() + params.sample_count * t_id,
                    read_counts.data() + t_id,
                    results.data() + t_id
            });
            threads.emplace_back([&params, &thread_params, &fn, t_id]() { fn(params, thread_params[t_id]); });
        }
        clock::time_point start_time = clock::now();
        barrier.wait();

        for (auto &thread : threads) {
            thread.join();
        }
        clock::time_point end_time = clock::now();
        auto read_sum = 0ul;
        for (auto& read : read_counts) {
            read_sum += read;
        }
        return RunResults{
            durations,
            std::chrono::duration_cast<time_resolution_t>(end_time - start_time),
            read_sum
        };
    }
    // --------------------------------------------------------------------------------
    void RamBenchmark::run_full_benchmark(RamLogger &logger) const {
        // Prepare memory spaces for the benchmark
        auto space_words = (config.payloads_max * static_cast<size_t>(config.threads_max)) / sizeof(size_t);
        auto read_space1 = new size_t[space_words];
        auto read_space2 = new size_t[space_words];
        for (size_t i = 0, size = space_words; i != size; ++i) {
           read_space1[i] = i;
           read_space2[i] = size - i;
        }
        // TODO: consider config.payloads_step
        auto params = RunParameters{ config.samples, 1, 0,  };
        for (size_t payload_size = config.payloads_min; payload_size <= config.payloads_max; payload_size *= 2) {
            params.payload_words = payload_size / sizeof(size_t);
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
        delete[] read_space1;
        delete[] read_space2;
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram
