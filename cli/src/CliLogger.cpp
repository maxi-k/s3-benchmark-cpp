//
// Created by maxi on 08.06.20.
//

#include "s3benchmark/cli/CliLogger.hpp"
#include "s3benchmark/Util.hpp"

namespace s3benchmark {

    CliLogger::CliLogger(std::ostream &output) : out(output) {}

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

    void CliLogger::print_run_results(const RunParameters &params, const RunResults &results) const {
        double time_s = results.overall_time.count() * 1.0 / units::ms_per_sec;
        double downloaded_mb = (results.data_points.size() * params.payload_size * 1.0) / units::mib;
        double throughput_mbps = downloaded_mb / time_s;
        out << format::string_format("| %7d | \033[1;31m%9.1f MB/s\033[0m |  ---   ---   ---   ---   ---   ---   ---   --- | ---   ---   ---   ---   ---   ---   ---   ---  |",
                                     params.thread_count,
                                     throughput_mbps) // TODO: include ttfb/ttlb values
                << std::endl;

    }

    void CliLogger::print_run_params(const RunParameters &params) const {
        auto bytes = format::byte_format(params.payload_size * 1.0);
        out << "Download performance with \033[1;33m" <<  bytes << "\033[0m objects\n" << std::endl;
    }


    void CliLogger::print_config_params(const ConfigParameters &config) const {
        out << "\n+------------------- \033[1;32mRUN CONFIGURATION\033[0m -------------------+\n";
        auto end_str = "+---------------------------------------------------------+\n";
        auto dry_run = config.dry_run ? "Yes" : "No";
        auto payload_dir = config.payloads_reverse ? "Backward" : "Forward";

        print_conf_var("Dry Run Only", dry_run);
        print_conf_var("EC2 Region", config.region);
        print_conf_var("Bucket Name", config.bucket_name);
        print_conf_var("Object Name", config.object_name);
        print_conf_var("Payloads Min", config.payloads_min);
        print_conf_var("Payloads Max", config.payloads_max);
        print_conf_var("Payloads Step", config.payloads_step);
        print_conf_var("Payloads Dir", payload_dir);
        print_conf_var("Threads Min", config.threads_min);
        print_conf_var("Threads Max", config.threads_max);
        print_conf_var("Threads Step", config.threads_step);
        print_conf_var("Sample Count", config.samples);
        out << end_str;

        out << "\n+------------------- \033[1;32mDETECTED HARDWARE\033[0m -------------------+\n";
        auto hw_threads = hardware::thread_count();
        print_conf_var("Detected Cores", hw_threads);
        // print_conf_var("Detected Instance Type", config.instance_type);
        out << end_str;
    }

}  // namespace s3benchmark::cli
