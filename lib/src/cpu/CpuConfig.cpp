//
// Created by maxi on 14.07.20.
//
#include "benchmark/cpu/CpuBenchmark.hpp"

namespace benchmark::cpu {

    CpuConfig::CpuConfig(Config &&config) : Config(std::move(config)) { }

}  // namespace benchmark::cpu

