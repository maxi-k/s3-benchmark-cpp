//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
namespace benchmark::ram {
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_header() const {
        out << "                                                      +-----------------------------+--------------------+" << std::endl;
        out << "                                                      |     Completion Time [ms]    |    Sample Count    |" << std::endl;
        out << "+---------+----------------+-----------+--------------+-----------------------------+--------------------+" << std::endl;
        out << "| Threads |      Bandwidth | Exec Time | GiB Read Sum |  min   max   avg   overall  |  thread   overall  |" << std::endl;
        out << "+---------+----------------+-----------+--------------+-----------------------------+--------------------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_footer() const {
        out << "+---------+----------------+-----------+--------------+-----------------------------+--------------------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_stats(const RunStats &stats) const {
        out << format::string_format("| %7d ", stats.thread_count)
            << format::string_format("| \033[1;31m%8.2f GiB/s\033[0m ", stats.bandwidth_gib_s)
            << format::string_format("|\033[1m%8.2f s\033[0m ", stats.duration.sum.count() * 1.0 / units::ms_per_sec)
            << format::string_format("|\033[1m%9.0f GiB\033[0m ", stats.read_sum * 1.0 / units::gib)
            << format::string_format("|%5d %5d %5d %9d  ", stats.duration.min.count(), stats.duration.max.count(), stats.duration.avg.count(), stats.duration.sum.count())
            << format::string_format("|%8d %9d  |", stats.sample_count, stats.samples_sum)
            << std::endl;
    }
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_params(const RunParameters &params) const {
        auto bytes = format::byte_format(params.payload_size * 1.0);
        out << "\nRead Bandwidth with \033[1;33m" <<  bytes << "\033[0m chunks\n" << std::endl;
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram

