//
// Created by Maximilian Kuschewski on 13.06.20.
//

#ifndef _S3BENCHMARK_HTTPCLIENT_HPP
#define _S3BENCHMARK_HTTPCLIENT_HPP

#include <string>
#include <functional>

namespace s3benchmark {

    class HttpClient {
        using response_handler_t = std::function<void(/* TODO */)>;
        size_t request_header_size;
        char* request_headers;
        size_t dynamic_header_index;
        response_handler_t response_handler;
        int socket_descriptor;

    public:
        HttpClient(const std::string &initial_request_headers,
                   size_t dynamic_header_index,
                   const response_handler_t &response_handler);

        ~HttpClient();

        [[nodiscard]] char *dynamic_header() const;

        [[nodiscard]] size_t header_size() const;

        void open_connection(const std::string &url);

        void execute_request(const size_t &excpected_payload_length) const;

        void close_connection();
    };

}  // namespace s3benchmark

#endif  //_S3BENCHMARK_HTTPCLIENT_HPP
