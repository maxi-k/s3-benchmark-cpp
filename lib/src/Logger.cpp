//
// Created by maxi on 14.07.20.
//
#include "benchmark/Logger.hpp"

namespace benchmark {

    void Logger::print_config_params(const ConfigParameters &config) const {
        auto dry_run = config.dry_run ? "Yes" : "No";
        auto payload_dir = config.payloads_reverse ? "Backward" : "Forward";

        out << "\n+---------------- \033[1;32mGENERAL CONFIGURATION\033[0m ------------------+\n";
        print_conf_var("Run Benchmark", BENCH_TYPE_NAMES[config.bench_type]);
        print_conf_var("Dry Run Only", dry_run);
        print_conf_var("Threads Min", config.threads_min);
        print_conf_var("Threads Max", config.threads_max);
        print_conf_var("Threads Step", config.threads_step);
        print_conf_var("Sample Count", config.samples);
        out << "+------------------- \033[1;32mS3 CONFIGURATION\033[0m --------------------+\n";
        print_conf_var("EC2 Region", config.region);
        print_conf_var("Bucket Name", config.bucket_name);
        print_conf_var("Object Name", config.object_name);
        print_conf_var("Payloads Min", config.payloads_min);
        print_conf_var("Payloads Max", config.payloads_max);
        print_conf_var("Payloads Step", config.payloads_step);
        print_conf_var("Payloads Dir", payload_dir);
        out << "+------------------- \033[1;32mRAM CONFIGURATION\033[0m -------------------+\n";
        print_conf_var("Ram Test Mode", RAM_MODE_NAMES[config.ram_mode]);
        print_conf_var("Payloads Min", config.payloads_min);
        print_conf_var("Payloads Max", config.payloads_max);
        print_conf_var("Payloads Step", config.payloads_step);
        out << "+---------------------------------------------------------+\n\n";
    }

    void Logger::print_ec2_config(const EC2Config &config) const {
        out << "\n+------------------- \033[1;32mDETECTED HARDWARE\033[0m -------------------+\n";
        print_conf_var("HW Threads", config.hw_thread_count);
        print_conf_var("Instance Id", config.ec2_instance_id);
        print_conf_var("Instance Type", config.ec2_instance_type);
        print_conf_var("AWS Region", config.ec2_region);
        out << "+---------------------------------------------------------+\n";
    }

}  // namespace benchmark
