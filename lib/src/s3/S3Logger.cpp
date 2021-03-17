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

    // ------------------------------------------------------------------------------------

    inline size_t steady_clock_to_mys(const clock::time_point& t) {
        return std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
    }

    void S3Logger::print_csv_run_header() const {
      out << "threads,read_byte,overall_us,start_time,end_time,diff_us" << std::endl;
    }

    void S3Logger::print_csv_run_detail(const RunParameters &params, const RunResults &results) const {
      size_t overall_us = std::chrono::duration_cast<std::chrono::microseconds>(results.overall_time).count();
      for (auto &[start, end] : results.data_points) {
        std::cout << params.thread_count << ","
                  << params.payload_size << ","
                  << overall_us << ","
                  << steady_clock_to_mys(start) << ","
                  << steady_clock_to_mys(end) << ","
                  << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
      }
    }

    void S3Logger::print_csv_run_footer() const {
        // NO-OP
    }

}  // namespace benchmark::s3
