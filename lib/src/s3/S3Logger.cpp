//
// Created by maxi on 08.06.20.
//

#include "benchmark/s3/S3Benchmark.hpp"
#include "benchmark/Util.hpp"

namespace benchmark::s3 {

    void S3Logger::print_run_header() const {
        out << "                                                      +-----------------------------+--------------------+" << std::endl;
        out << "                                                      |     Completion Time [ms]    |    Sample Count    |" << std::endl;
        out << "+---------+----------------+-----------+--------------+-----------------------------+--------------------+" << std::endl;
        out << "| Threads |     Throughput | Exec Time | Download Sum |  min   max   avg   overall  |  thread   overall  |" << std::endl;
        out << "+---------+----------------+-----------+--------------+-----------------------------+--------------------+" << std::endl;
    }

    void S3Logger::print_run_footer() const {
        out << "+---------+----------------+-----------+--------------+-----------------------------+--------------------+" << std::endl;
    }

    void S3Logger::print_run_stats(const s3::RunStats &stats) const {
        out << format::string_format("| %7d ", stats.thread_count)
            << format::string_format("| \033[1;31m%8.2f MiB/s\033[0m ", stats.throughput_mbps)
            << format::string_format("|\033[1m%8.2f s\033[0m ", stats.duration.count() * 1.0 / units::ms_per_sec)
            << format::string_format("|\033[1m%9.0f MiB\033[0m ", stats.download_sum * 1.0 / units::mib)
            << format::string_format("|%5d %5d %5d %9d  ", stats.latency_min.count(), stats.latency_max.count(), stats.latency_avg.count(), stats.latency_sum.count())
            << format::string_format("|%8d %9d  |", stats.sample_count, stats.samples_sum)
            << std::endl;
    }

    void S3Logger::print_run_params(const s3::RunParameters &params) const {
        auto bytes = format::byte_format(params.payload_size * 1.0);
        out << "\nDownload performance with \033[1;33m" <<  bytes << "\033[0m objects\n" << std::endl;
    }

}  // namespace benchmark::s3
