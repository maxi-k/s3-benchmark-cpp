//
// Created by maxi on 08.06.20.
//

#ifndef _S3BENCHMARK_LOGGER_HPP
#define _S3BENCHMARK_LOGGER_HPP

#include <ostream>
#include "Types.hpp"
#include "Config.hpp"

namespace s3benchmark {
class Logger {
protected:
    std::ostream &out;
public:
    explicit Logger(std::ostream &output) : out(output) {};
    virtual void print_run_header() const = 0;
    virtual void print_run_footer() const = 0;
    virtual void print_run_results(RunParameters &params, RunResults &results) const = 0;
    virtual void print_run_params(RunParameters &params) const = 0;
    virtual void print_config_params(ConfigParameters &config) const = 0;
};

}  // namespace s3benchmark::cli


#endif  // _S3BENCHMARK_LOGGER_HPP
