//
// Created by maxi on 14.07.20.
//
#include "benchmark/cpu/CpuBenchmark.hpp"

namespace benchmark::cpu {
   // --------------------------------------------------------------------------------
    void CpuLogger::print_run_header() const {
       out << "+------------+---------+-----------+" << std::endl;
       out << "|  Est. GHz  | GigaOps | Exec Time |" << std::endl;
       out << "+------------+---------+-----------+" << std::endl;
   }
    // --------------------------------------------------------------------------------
    void CpuLogger::print_run_footer() const {
        out << "+------------+---------+-----------+" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void CpuLogger::print_run_params(const RunParameters &params) const {
        out << "\nTesting with \033[1;33m" << params.samples
            << "\033[0m samples and \033[1;33m" << params.num_ops
            << "\033[0m ops per sample.\n" << std::endl;
    }
    // --------------------------------------------------------------------------------
    void CpuLogger::print_run_stats(const RunStats &stats) const {
        out << format::string_format("|  \033[1;31m%4.2f GHz\033[0m  ", stats.clock_speed)
            << format::string_format("| %7.2f ", 1.0 * stats.total_ops / units::giga)
            << format::string_format("| %8.3fs |", stats.total_duration)
            << std::endl;
    }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::cpu

