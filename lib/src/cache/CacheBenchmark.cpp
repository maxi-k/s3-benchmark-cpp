//
// Created by maxi on 21.07.20.
//
#include <algorithm>
#include <random>
#include "benchmark/cache/CacheBenchmark.hpp"

namespace benchmark::cache {
    // --------------------------------------------------------------------------------
    uint64_t rdtsc() {
        uint32_t hi, lo;
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
        return static_cast<uint64_t>(lo)|(static_cast<uint64_t>(hi)<<32);
    }
    // --------------------------------------------------------------------------------
    CacheBenchmark::CacheBenchmark(const CacheConfig &config) : config(config) {}
    // --------------------------------------------------------------------------------
    RunResults CacheBenchmark::do_run(const RunParameters &params, size_t* memspace) const {
        std::vector<size_t> durations(params.thread_count);
        std::vector<size_t> read_counts(params.thread_count);
        std::vector<size_t> results(params.thread_count);

        Barrier barrier(params.thread_count + 1);
        std::vector<std::thread> threads;
        for (size_t t_id = 0; t_id != params.thread_count; ++t_id) {
            threads.emplace_back([&, t_id]() {
                auto rep = params.sample_count;
                auto jumps = params.linkset_jumps;
                size_t cache[128]; // TODO: check size
                barrier.wait();
                auto start = rdtsc();
                for (unsigned j = 0; j != jumps; ++j) {
                    cache[j] = j;
                }
                for (unsigned i = 0; i != rep; ++i) {
                   for (unsigned j = 0; j != jumps; ++j)  {
                       cache[j] = memspace[cache[j]];
                   }
                }
                size_t sum = 0;
                for (unsigned j = 0; j < jumps; ++j) {
                    sum += cache[j];
                }
                auto end  = rdtsc();
                durations[t_id] = (end - start) / rep;
                read_counts[t_id] = rep * jumps * sizeof(size_t);
                results[t_id] = sum;
            });

        }

        auto time_start = clock::now();
        barrier.wait();
        for (auto &thread : threads) {
            thread.join();
        }
        auto time_end = clock::now();

        auto read_sum = 0ul, result = 0ul;
        for (unsigned i = 0; i != params.thread_count; ++i) {
            read_sum += read_counts[i];
            result += results[i] ;
        }
        return RunResults{
                durations,
                time_end - time_start,
                read_sum,
                result
        };
    }
    // --------------------------------------------------------------------------------
    void CacheBenchmark::run_full_benchmark(CacheLogger &logger) const {
        auto params = RunParameters{config.samples, 1, 0, 0};
        for (size_t payload_size = config.payloads_min; payload_size <= config.payloads_max; payload_size *= config.payloads_step) {
            // prepare random jump list to avoid out of order execution
            size_t space_words = (params.payload_words = payload_size / sizeof(size_t));
            auto shuffle = new size_t[space_words];
            for (size_t i = 0; i != space_words; ++i) {
                shuffle[i] = i;
            }
            // replacement for std::random_shuffle in c++17
            std::shuffle(shuffle, shuffle + space_words, std::mt19937(std::random_device()()));
            auto memspace = new size_t[space_words];
            for (size_t i = 0; i < space_words; ++i) {
                memspace[shuffle[i]] = shuffle[(i + 1) % space_words];
            }
            for (size_t read_count = config.cache_reads_min; read_count <= config.cache_reads_max; read_count *= config.cache_reads_step) {
                params.linkset_jumps = read_count;
                logger.print_run_params(params);
                logger.print_run_header();
                for (size_t thread_count = config.threads_min;
                     thread_count <= config.threads_max; thread_count *= config.threads_step) {
                    params.thread_count = thread_count;
                    auto results = this->do_run(params, memspace);
                    auto stats = RunStats(params, results);
                    logger.print_run_stats(stats);
                }
                logger.print_run_footer();
            }
            delete[] shuffle;
            delete[] memspace;
        }
    }
// --------------------------------------------------------------------------------
}
