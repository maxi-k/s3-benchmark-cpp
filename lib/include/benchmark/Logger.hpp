//
// Created by maxi on 08.06.20.
//

#ifndef _BENCHMARK_LOGGER_HPP
#define _BENCHMARK_LOGGER_HPP

#include <ostream>
#include "Config.hpp"

namespace benchmark {
    class Logger {
    protected:
        std::ostream &out;
        template<typename S, typename T> inline void print_conf_var(S &name, T &var) const  {
            out << "| \033[1m" << name << "\033[0m\t\t:\t" << var << "\033[59G|\n";
        }
    public:
        explicit Logger(std::ostream &stream) : out(stream) {}
        void print_config_params(const ConfigParameters &config) const;
        void print_ec2_config(const EC2Config &config) const;
    };
}  // namespace benchmark

#endif  // _BENCHMARK_LOGGER_HPP
