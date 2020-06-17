//
// Created by Maximilian Kuschewski on 07.06.20.
//

#ifndef _S3BENCHMARK_UTIL_HPP
#define _S3BENCHMARK_UTIL_HPP

#include <thread>
#include <random>
#include <iostream>

#include <curl/curl.h>

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
            inline int overflow(int c) override {
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

    namespace predicate {
        inline bool starts_with(const char* s1, const char* s2) {
            if (*s1 == '\0' || *s2 == '\0') return true;
            while(*s1 == *s2) {
                ++s1; ++s2;
                if (*s1 == '\0' || *s2 == '\0') return true;
            }
            return false;
        }

        #include<iostream>
        #include<string>

        // Adapted from https://gist.github.com/YDrall/d782a03430d5002cc3bc
        inline size_t* pre_kmp(const std::string &pattern) {
            auto size = pattern.size();
            auto *pie = new size_t[size];
            pie[0] = 0;
            size_t k = 0;
            for (size_t i = 1; i < size; i++) {
                while (k > 0 && pattern[k] != pattern[i]) {
                    k = pie[k - 1];
                }
                if (pattern[k] == pattern[i]) {
                    k = k + 1;
                }
                pie[i] = k;
            }
            return pie;
        }

        // Adapted from https://gist.github.com/YDrall/d782a03430d5002cc3bc
        // returns [ last_match_pos, n_matches ]
        inline std::pair<size_t, size_t> find_kmp(const char* text, const size_t &text_length, const std::string &pattern) {
            size_t pattern_length = pattern.length();
            size_t* pie = pre_kmp(pattern);
            size_t matched_pos = 0;
            size_t matches = 0;
            for(size_t current = 0; current < text_length; current++) {
                while (matched_pos > 0 && pattern[matched_pos] != text[current]) {
                    matched_pos = pie[matched_pos - 1];
                }
                if(pattern[matched_pos] == text[current]) {
                    matched_pos = matched_pos + 1;
                }
                if (matched_pos == pattern_length) {
                    ++matches;
                    matched_pos = pie[matched_pos - 1];
                }
            }
            delete pie;
            return { matched_pos, matches };
        }

        inline bool find_next(char** haystack_ptr, const char* end, const char* needle, const char* needle_end) {
            auto needle_size = needle_end - needle;
            auto haystack = *haystack_ptr;
            // TODO: optimize, int comparison / vector instructions?
            while (haystack < end - needle_size + 1) {
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
    }

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

        inline size_t curl_callback(char *body, size_t size_mult, size_t nmemb, void *userdata) {
            auto size = size_mult * nmemb;
            static_cast<std::string*>(userdata)->append(body, size);
            return size;
        }

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
            curl_easy_cleanup(curl);
            if (res == 0) {
                return body;
            } else {
                throw std::runtime_error("Failed fetching instance id.");
            }
        }
    }
}

#endif //_S3BENCHMARK_UTIL_HPP
