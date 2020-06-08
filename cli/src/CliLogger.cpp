//
// Created by maxi on 08.06.20.
//

#include "s3benchmark/cli/CliLogger.hpp"
#include "s3benchmark/Util.hpp"

namespace s3benchmark {

    CliLogger::CliLogger(std::ostream &output) : Logger(output) {}

    void CliLogger::print_run_header() const {
        out << "                           +-------------------------------------------------------------------------------------------------+" << std::endl;
        out << "                           |            Time to First Byte (ms)             |            Time to Last Byte (ms)              |" << std::endl;
        out << "+---------+----------------+------------------------------------------------+------------------------------------------------+" << std::endl;
        out << "| Threads |     Throughput |  avg   min   p25   p50   p75   p90   p99   max |  avg   min   p25   p50   p75   p90   p99   max |" << std::endl;
        out << "+---------+----------------+------------------------------------------------+------------------------------------------------+" << std::endl;
    }

    void CliLogger::print_run_footer() const {
        out << "+---------+----------------+------------------------------------------------+------------------------------------------------+" << std::endl;
    }

    void CliLogger::print_run_results(RunParameters &params, RunResults &results) const {
        double time_s = results.overall_time.count() * 1.0 / units::ms_per_sec;
        double downloaded_mb = (results.data_points.size() * params.payload_size * 1.0) / units::mib;
        double throughput_mbps = downloaded_mb / time_s;
        out << format::string_format("| %7d | \033[1;31m%9.1f MB/s\033[0m |  ---   ---   ---   ---   ---   ---   ---   --- | ---   ---   ---   ---   ---   ---   ---   ---  |",
                                     params.thread_count,
                                     throughput_mbps) // TODO: include ttfb/ttlb values
                << std::endl;

    }

    void CliLogger::print_run_params(RunParameters &params) const {
        auto bytes = format::byte_format(params.payload_size * 1.0);
        out << "Download performance with \033[1;33m" <<  bytes << "\033[0m objects\n" << std::endl;
    }

    void CliLogger::print_config_params(ConfigParameters &config) const {

    }

}  // namespace s3benchmark::cli
