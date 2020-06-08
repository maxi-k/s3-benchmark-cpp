//
// Created by maxi on 08.06.20.
//

#ifndef _S3BENCHMARK_CLILOGGER_HPP
#define _S3BENCHMARK_CLILOGGER_HPP

#include "s3benchmark/Logger.hpp"
#include "s3benchmark/Types.hpp"

namespace s3benchmark {
    class CliLogger : public Logger {
    protected:
    public:
        explicit CliLogger(std::ostream &output);
        void print_run_header() const override;
        void print_run_footer() const override;
        void print_run_results(RunParameters &params, RunResults &results) const override;
        void print_run_params(RunParameters &params) const override;
        void print_config_params(ConfigParameters &config) const override;
    };
}  // namespace s3benchmark::cli

#endif  //_S3BENCHMARK_CLILOGGER_HPP
