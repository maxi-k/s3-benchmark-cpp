//
// Created by maxi on 21.07.20.
//
#include "benchmark/cache/CacheBenchmark.hpp"

namespace benchmark::cache {
    // --------------------------------------------------------------------------------
    void CacheLogger::print_run_header() const {
        out << "                                                             +-----------------------------+--------------------+" << std::endl;
        out << "                                                             |        Cycles per Run       |    Sample Count    |" << std::endl;
        out << "+---------+----------------+----------------+----------------+-----------------------------+--------------------+" << std::endl;
        out << "| Threads |      Bandwidth | Exec Time [ns] | Read Sum [KiB] |  min   max   avg   overall  |  thread   overall  |" << std::endl;
        out << "+---------+----------------+----------------+----------------+-----------------------------+--------------------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void CacheLogger::print_run_footer() const {
        out << "+---------+----------------+----------------+----------------+-----------------------------+--------------------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void CacheLogger::print_run_stats(const RunStats &stats) const {
        out << format::string_format("| %7d ", stats.thread_count)
            << format::string_format("| \033[1;31m%8.2f GiB/s\033[0m ", stats.bandwidth_gib_s)
            << format::string_format("|\033[1m%15.2f\033[0m ", units::duration_nanoseconds(stats.overall_duration / stats.sample_count).count())
            << format::string_format("|\033[1m%15.2f\033[0m ", stats.read_sum_bytes * 1.0 / units::kib)
            << format::string_format("|%5d %5d %5d %9d  ",
                                     stats.cycles.min,
                                     stats.cycles.max,
                                     stats.cycles.avg,
                                     stats.cycles.sum)
            << format::string_format("|%8d %9d  |", stats.sample_count, stats.samples_sum)
            << std::endl;
    }
    // --------------------------------------------------------------------------------
    void CacheLogger::print_run_params(const RunParameters &params) const {
        auto bytes = format::byte_format(params.payload_words * sizeof(size_t) * 1.0);
        out << "\nRead Bandwidth with \033[1;33m" << bytes << "\033[0m chunks and "
            << "\033[1;33m" << params.linkset_jumps << "\033[0m reads per sample."
            << std::endl;
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cache
