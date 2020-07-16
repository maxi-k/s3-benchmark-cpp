//
// Created by maxi on 15.07.20.
//
#include "benchmark/ram/RamBenchmark.hpp"
namespace benchmark::ram {
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_header() const {
        out << "                                                             +-----------------------------+--------------------+" << std::endl;
        out << "                                                             |     Completion Time [ms]    |    Sample Count    |" << std::endl;
        out << "+---------+----------------+----------------+----------------+-----------------------------+--------------------+" << std::endl;
        out << "| Threads |      Bandwidth | Exec Time [ms] | Read Sum [MiB] |  min   max   avg   overall  |  thread   overall  |" << std::endl;
        out << "+---------+----------------+----------------+----------------+-----------------------------+--------------------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_footer() const {
        out << "+---------+----------------+----------------+----------------+-----------------------------+--------------------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_stats(const RunStats &stats) const {
        out << format::string_format("| %7d ", stats.thread_count)
            << format::string_format("| \033[1;31m%8.2f GiB/s\033[0m ", stats.bandwidth_gib_s)
            << format::string_format("|\033[1m%15.2f\033[0m ", units::duration_milliseconds(stats.duration.sum).count())
            << format::string_format("|\033[1m%15.2f\033[0m ", stats.read_sum * 1.0 / units::mib)
            << format::string_format("|%5.1f %5.1f %5.1f %9.1f  ",
                    units::duration_milliseconds(stats.duration.min).count(),
                    units::duration_milliseconds(stats.duration.max).count(),
                    units::duration_milliseconds(stats.duration.avg).count(),
                    units::duration_milliseconds(stats.duration.sum).count())
            << format::string_format("|%8d %9d  |", stats.sample_count, stats.samples_sum)
            << std::endl;
    }
    // --------------------------------------------------------------------------------
    void RamLogger::print_run_params(const RunParameters &params) const {
        auto bytes = format::byte_format(params.payload_words * sizeof(size_t) * 1.0);
        out << "\nRead Bandwidth with \033[1;33m" <<  bytes << "\033[0m chunks\n" << std::endl;
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram

