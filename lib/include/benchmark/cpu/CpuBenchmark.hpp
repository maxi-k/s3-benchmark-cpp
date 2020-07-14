//
// Created by maxi on 14.07.20.
//

#ifndef _BENCHMARK_CPUBENCHMARK_H
#define _BENCHMARK_CPUBENCHMARK_H

class CpuBenchmark {
    CpuConfig config;
public:
    CpuBenchmark(CpuConfig &config)
    [[nodiscard]] RunResults do_run(RunParameters &params) const;
    void run_full_benchmark(S3Logger &logger) const;
};

#endif //_BENCHMARK_CPUBENCHMARK_H
