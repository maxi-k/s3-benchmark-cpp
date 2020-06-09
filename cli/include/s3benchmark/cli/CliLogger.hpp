//
// Created by maxi on 08.06.20.
//

#ifndef _S3BENCHMARK_CLILOGGER_HPP
#define _S3BENCHMARK_CLILOGGER_HPP

#include "s3benchmark/Logger.hpp"
#include "s3benchmark/Types.hpp"

namespace s3benchmark {
    class CliLogger : public Logger {
        std::ostream &out;
        template<typename S, typename T> inline void print_conf_var(S &name, T &var) const  {
            out << "| \033[1m" << name << "\033[0m\t\t:\t" << var << "\033[59G|\n";
        }
    public:
        explicit CliLogger(std::ostream &output);
        void print_run_header() const override;
        void print_run_footer() const override;
        void print_run_stats(const RunStats &stats) const override;
        void print_run_params(const RunParameters &params) const override;
        void print_config_params(const ConfigParameters &config) const override;
        void print_ec2_config(const EC2Config &config) const override;
    };
}  // namespace s3benchmark::cli

#endif  //_S3BENCHMARK_CLILOGGER_HPP
