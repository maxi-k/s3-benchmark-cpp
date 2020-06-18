//
// Created by maxi on 08.06.20.
//

#ifndef _S3BENCHMARK_TYPES_HPP
#define _S3BENCHMARK_TYPES_HPP

#include <sys/select.h>
#include <chrono>
#include <ratio>
#include <vector>
#include <string>
#include <sstream>
#include <netdb.h>
#include <unistd.h>
#include "Util.hpp"

namespace s3benchmark {
    using clock = std::chrono::steady_clock;
    using timestamp_t = clock::time_point;
    using latency_t = std::chrono::duration<size_t, std::ratio<1, 1000>>;

    struct ReadStats {
        size_t chunk_count;
        size_t payload_size;
        latency_t read_duration;
    };

    struct ByteRange {
        size_t first_byte;
        size_t last_byte;

        inline std::string as_http_header() const {
            std::ostringstream s;
            s << "bytes=" << first_byte << "-" << last_byte;
            return s.str();
        }

        inline std::string as_string() const {
            std::ostringstream s;
            s << first_byte << "-" << last_byte;
            return s.str();
        }

        inline size_t length() const {
            return last_byte - first_byte;
        }
    };

    struct RunParameters {
        size_t sample_count;
        size_t thread_count;
        size_t payload_size;
        size_t content_size;
    };

    struct RunResults {
        std::vector<latency_t> latencies;
        std::vector<size_t> payload_sizes;
        std::vector<size_t> chunk_counts;
        latency_t overall_time;
    };

    template<typename T>
    struct ValueStats {
        T avg;
        T sum;
        T min;
        T max;

        ValueStats(const std::vector<T> &data_points, const T &zero_val) {
            T v_min, v_max = data_points[0];
            T v_sum = zero_val;
            for (auto& dp : data_points) {
                if (dp < v_min) v_min = dp;
                if (dp > v_max) v_max = dp;
                v_sum += dp;
            }
            this->min = v_min;
            this->max = v_max;
            this->sum = v_sum;
            this->avg = v_sum / data_points.size();
        }
    };

    struct RunStats : public RunParameters {
        double throughput_http_mbps;
        double throughput_tcp_mbps;
        latency_t duration;
        size_t download_sum;
        size_t samples_sum;

        ValueStats<latency_t> latency;
        ValueStats<size_t> chunk_count;
        ValueStats<size_t> payload_size;

        RunStats(const RunParameters &params, const RunResults &run)
            : RunParameters(params)
            , samples_sum(run.latencies.size())
            , duration(run.overall_time)
            , latency(run.latencies, latency_t::zero())
            , chunk_count(run.chunk_counts, 0ul)
            , payload_size(run.payload_sizes, 0ul) {
            this->download_sum = params.thread_count * params.sample_count * params.payload_size;
            this->throughput_http_mbps = (download_sum * 1.0 / units::mib) / (duration.count() * 1.0 / units::ms_per_sec);
            this->throughput_tcp_mbps = (payload_size.sum * 1.0 / units::mib) / (duration.count() * 1.0 / units::ms_per_sec);
        }
    };

    struct Connection {
        size_t id;
        int socket;
        long pending_bytes;
        int writes;
        timestamp_t last_write;
        timestamp_t last_read;
        inline bool is_open() const {
            return socket != -1;
        }
        inline void close_connection() {
            if (is_open()) {
                close(this->socket);
            }
            this->socket = -1;
        }
        template<typename T>
        static inline fd_set make_fd_set(const T &conn_list) {
            fd_set set;
            FD_ZERO(&set);
            for (auto &conn : conn_list) {
                if (conn.is_open()) {
                    FD_SET(conn.socket, &set);
                }
            }
            return set;
        }
    };

    struct TestEnv {
        std::vector<Connection> connections;
        std::vector<std::string> requests;
        hostent *remote_host;

        int set_count;
        fd_set send_set_all;
        fd_set recv_set_all;

        size_t overall_sample_count;
        size_t executed_requests;
        size_t received_responses;
        size_t received_bytes;
    };
}

#endif  // _S3BENCHMARK_TYPES_HPP
