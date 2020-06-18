//
// Created by Maximilian Kuschewski on 13.06.20.
//

#ifndef _S3BENCHMARK_HTTPCLIENT_HPP
#define _S3BENCHMARK_HTTPCLIENT_HPP

#include <netdb.h>
#include <string>
#include <functional>
#include "Config.hpp"
#include "Types.hpp"

namespace s3benchmark {

    class HttpClient {
    public:
        using response_handler_t = std::function<void(Connection&, size_t, char*)>;
        HttpClient();

        ~HttpClient();

        static ReadStats receive_msg(Connection &conn, const size_t &buffer_size, char* recv_buffer, const response_handler_t &handler);
        static void send_msg(Connection &conn, const std::string &msg);
        static hostent* lookup_host(const std::string &url);
        static Connection create_connection(const hostent* host, size_t id = 0ul);
    };

}  // namespace s3benchmark

#endif  //_S3BENCHMARK_HTTPCLIENT_HPP
