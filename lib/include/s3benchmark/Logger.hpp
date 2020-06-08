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
public:
    virtual void print_run_header() const {}
    virtual void print_run_footer() const {}
    virtual void print_run_results(const RunParameters &params, const RunResults &results) const {}
    virtual void print_run_params(const RunParameters &params) const {}
    virtual void print_config_params(const ConfigParameters &config) const {}
};

}  // namespace s3benchmark::cli


#endif  // _S3BENCHMARK_LOGGER_HPP
