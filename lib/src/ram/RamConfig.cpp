//
// Created by maxi on 15.07.20.
//
#include <utility>
#include "benchmark/ram/RamBenchmark.hpp"

namespace benchmark::ram {
    // --------------------------------------------------------------------------------
    RamConfig::RamConfig(Config &&config)
        : Config(std::move(config))
        // TODO base on hw cache size
        , cache_size(10 * units::mib) { }
    // --------------------------------------------------------------------------------
}  // namespace benchmark::ram
