//
// Created by Maximilian Kuschewski on 07.06.20.
//

#ifndef _BENCHMARK_UTIL_HPP
#define _BENCHMARK_UTIL_HPP

#include <thread>
#include <random>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <curl/curl.h>

namespace benchmark {
    // --------------------------------------------------------------------------------
    using clock = std::chrono::steady_clock;
    // --------------------------------------------------------------------------------
    namespace hardware {
        inline size_t thread_count() noexcept {
            return std::max(std::thread::hardware_concurrency(), 1u);
        }
    }
    // --------------------------------------------------------------------------------
    namespace units {
        inline static const size_t kib = 1024;
        inline static const size_t mib = 1024 * kib;
        inline static const size_t gib = 1024 * mib;

        inline static const size_t kilo = 1000;
        inline static const size_t mega = 1000 * kilo;
        inline static const size_t giga = 1000 * mega;

        inline static const size_t ms_per_sec  = 1000;
        inline static const size_t mys_per_sec = 1000 * ms_per_sec;
        inline static const size_t ns_per_sec  = 1000 * mys_per_sec;

        inline static const size_t ms_per_min = 60 * ms_per_sec;

        template<typename ToPeriod, typename FromDuration>
        inline std::chrono::duration<double, ToPeriod> duration_float(FromDuration from) {
            std::chrono::duration<double, ToPeriod> result = from;
            return result;
        }

        template<typename FromDuration>
        inline std::chrono::duration<double, std::chrono::seconds::period> duration_seconds(FromDuration from) {
            return duration_float<std::chrono::seconds::period>(from);
        }

        template<typename FromDuration>
        inline std::chrono::duration<double, std::chrono::milliseconds::period> duration_milliseconds(FromDuration from) {
            return duration_float<std::chrono::milliseconds::period>(from);
        }

        template<typename FromDuration>
        inline std::chrono::duration<double, std::chrono::nanoseconds ::period> duration_nanoseconds(FromDuration from) {
            return duration_float<std::chrono::nanoseconds::period>(from);
        }
    }
    // --------------------------------------------------------------------------------
    namespace random {
        template<typename T>
        inline T in_range(T min, T max) {
            thread_local static std::mt19937 engine {std::random_device{}()};
            thread_local static std::uniform_int_distribution<T> dist(min, max);
            return dist(engine);
        };
    }
    // --------------------------------------------------------------------------------
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
    // --------------------------------------------------------------------------------
    namespace predicate {
        // TODO implement more efficiently
        inline bool find_next(char** haystack_ptr, const char* end, const char* needle, const char* needle_end) {
            auto needle_size = needle_end - needle;
            auto haystack = *haystack_ptr;
            // TODO: optimize, int comparison / vector instructions?
            while (haystack < end - needle_size + 0) {
                const char* comp = needle;
                while(comp != needle_end && haystack != end && (*haystack++) == (*comp++));
                if (comp == needle_end && *(comp-1) == *(haystack-1)) {
                    *haystack_ptr = haystack;
                    return true;
                }
            }
            *haystack_ptr = const_cast<char *>(end);
            return false;
        }
    }  // namespace predicate
    // --------------------------------------------------------------------------------
    namespace http {
        // [offset in next buffer, n_matches]
        inline std::pair<size_t, size_t> skim_http_data(char* buf, const size_t &buf_len, const size_t &start_pos = 0) {
            static const char search_string[] = "Content-Length: ";
            static const size_t search_string_len = sizeof(search_string) - 1;
            if (start_pos >= buf_len) {
                return {start_pos - buf_len, 0};
            }
            size_t matches = 0;
            const char* end = buf + buf_len;
            char* pos = buf + start_pos;
            while (pos < end) {
                bool found = predicate::find_next(&pos, end, search_string, search_string + search_string_len - 1);
                if (!found) {
                    return { 0, matches};
                }
                ++matches;
                if (pos == end) {
                    return {0, matches};
                }
                auto skip_bytes = strtol(pos, &pos, 10) + 6; // parsed content length + 6 for http \r\n's
                pos += skip_bytes;
            }
            return {pos - end, matches};
        }
        // --------------------------------------------------------------------------------
        inline size_t curl_callback(char *body, size_t size_mult, size_t nmemb, void *userdata) {
            auto size = size_mult * nmemb;
            static_cast<std::string*>(userdata)->append(body, size);
            return size;
        }
        // --------------------------------------------------------------------------------
        inline std::string curl_get(const std::string &url, size_t timeout_ms = 1000) {
            CURL *curl = curl_easy_init();
            if (!curl) {
                throw std::runtime_error("Could not initialize curl client.");
            }
            CURLcode res;
            std::string body;
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_callback);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
            res = curl_easy_perform(curl);
            if (res == 0) {
                return body;
            } else {
                throw std::runtime_error("Failed fetching instance id.");
            }
        }
    }
    // --------------------------------------------------------------------------------
    namespace input {
        // Code adapted from FDE bonus project
        // https://gitlab.db.in.tum.de/maxi-k/fde19-bonusproject-1/-/raw/master/include/Input.hpp
        class MemoryMappedFile {
            int handle = -1;
            uintptr_t size = 0;
            char *mapping = nullptr;

        public:
            MemoryMappedFile() {}

            MemoryMappedFile(const std::string &filename, const int &mode) { open(filename.data(), mode); }

            ~MemoryMappedFile() { close(); }

            inline bool open(const char *file, const int &mode) {
                close();

                // O_RDONLY, O_RDWR, O_WRONLY
                int h = ::open(file, mode);
                if (h < 0) return false;

                lseek(h, 0, SEEK_END);
                size = lseek(h, 0, SEEK_CUR);

                auto m = mmap(nullptr, size, PROT_READ, MAP_SHARED, h, 0);
                if (m == MAP_FAILED) {
                    ::close(h);
                    return false;
                }

                handle = h;
                mapping = static_cast<char *>(m);
                return true;
            }

            // TODO: evaluate whether this makes any difference for performance
            inline void prepareRead() {
                // From wc source: advise kernel that we will be reading the file
                /* Advise the kernel of our access pattern.  */
#ifdef __linux__
                posix_fadvise(handle, 0, 0, 1);  // FDADVICE_SEQUENTIAL
#endif
            }

            inline void close() {
                if (handle >= 0) {
                    munmap(mapping, size);
                    ::close(handle);
                    handle = -1;
                }
            }

            inline const char *begin() const {
                return mapping;
            }

            inline const char *end() const {
                return mapping + size;
            }
        };  // MemoryMappedFile
    }  // namespace input
    // --------------------------------------------------------------------------------
    template<typename T>
    struct ValueStats {
        size_t count;
        T avg;
        T sum;
        T min;
        T max;

        ValueStats(const std::vector<T> &data_points, const T &zero_val)
            : count(data_points.size()) {
            T v_min = data_points[0], v_max = data_points[0];
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
    };  // ValueStats

    // --------------------------------------------------------------------------------

    // Adapted from https://stackoverflow.com/questions/38999911/what-is-the-best-way-to-realize-a-synchronization-barrier-between-threads
    class Barrier {
        std::mutex latch;
        std::condition_variable condition;
        size_t waiting;
    public:
        Barrier(size_t max_waiting)
                : latch()
                , condition()
                , waiting(max_waiting) {
            if (max_waiting == 0) {
                throw std::runtime_error("Waiting for 0 threads is not allowed");
            }
        }
        Barrier(const Barrier& barrier) = delete;
        Barrier(Barrier&& barrier) = delete;
        Barrier& operator=(const Barrier& barrier) = delete;
        Barrier& operator=(Barrier&& barrier) = delete;
        ~Barrier() noexcept(false) {
            if (waiting != 0) {
                throw std::runtime_error("There are threads waiting on a barrier about to be destroyed.");
            }
        }

        void wait() {
            std::unique_lock<std::mutex> lock(latch);
            if(waiting == 0) {
               throw std::runtime_error("Waiting with too many threads on barrier.");
            }
            if (--waiting == 0) {
                condition.notify_all();
            } else {
                condition.wait(lock, [this]() {
                    return 0 == waiting;
                });
            }
        }
    };
}  // namespace benchmark

#endif //_BENCHMARK_UTIL_HPP
