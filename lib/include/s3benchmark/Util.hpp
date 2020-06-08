//
// Created by Maximilian Kuschewski on 07.06.20.
//

#ifndef _S3BENCHMARK_UTIL_HPP
#define _S3BENCHMARK_UTIL_HPP

#include <thread>
#include <random>
#include <iostream>

namespace s3benchmark {
    namespace hardware {
        inline size_t thread_count() noexcept {
            return std::max(std::thread::hardware_concurrency(), 1u);
        }
    }

    namespace units {
        inline static const size_t kib = 1024;
        inline static const size_t mib = 1024 * kib;
        inline static const size_t gib = 1024 * mib;

        inline static const size_t ms_per_sec = 1000;
        inline static const size_t ms_per_min = 60 * ms_per_sec;
    }

    namespace random {
        template<typename T>
        inline T in_range(T min, T max) {
            thread_local static std::mt19937 engine {std::random_device{}()};
            thread_local static std::uniform_int_distribution<T> dist(min, max);
            return dist(engine);
        };
    }

    namespace stream {
        class VoidBuffer : public std::streambuf {
        public:
            int overflow(int c) override {
                return c;
            }
        };
    }

    namespace format {
        // Not using c++ 20 std::format
        // from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
        template<typename ... Args>
        inline std::string string_format(const std::string& format, Args ... args) {
            size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
            if( size <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
            std::unique_ptr<char[]> buf( new char[ size ] );
            snprintf( buf.get(), size, format.c_str(), args ... );
            return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
        }

        inline std::string byte_format(double bytes) {
            if (bytes >= units::gib) {
                return string_format("%.f GiB", bytes / units::gib);
            }
            if (bytes >= units::mib) {
                return string_format("%.f MiB", bytes / units::mib);
            }
            if (bytes >= units::kib) {
                return string_format("%.f KiB", bytes / units::kib);
            }
            return string_format("%d", bytes);
        }
    }
}

#endif //_S3BENCHMARK_UTIL_HPP
