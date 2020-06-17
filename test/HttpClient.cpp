//
// Created by maxi on 15.06.20.
//
#include <s3benchmark/HttpClient.hpp>
#include <sys/select.h>
#include <gtest/gtest.h>

namespace s3benchmark {
#define DEBUG true
#define sout \
    if (!DEBUG) {} \
    else std::cerr

    TEST(HttpClient, no_dynamic_part)
    /// Test fetching from example.com
{
    std::string headers = "GET / HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";
    auto conn = HttpClient::create_connection(HttpClient::lookup_host("example.com"));
    auto conns = std::array{ conn };
    struct timeval timeout_def{ 1, 0 };
    auto poll_set = Connection::make_fd_set(conns);
    select(2, nullptr, &poll_set, nullptr, &timeout_def);
    HttpClient::send_msg(conn, headers);

    std::string expected_start = "HTTP/1.1 200 OK";
    bool result = false;
    std::vector<char> buf_start(expected_start.size());
    std::vector<char> outbuf(500);
    HttpClient::response_handler_t handler = [&result, &buf_start](const Connection &conn, size_t received, char* buffer) {
        if (!result) {
            memcpy(buf_start.data(), buffer, buf_start.size());
        }
        result = true;
        sout << "\n----------------\nReceived data with recv_len " << received << "\n----------------\n" << std::endl;
        sout << buffer << std::endl;
    };
    poll_set = Connection::make_fd_set(conns);
    timeout_def.tv_sec = 1;
    select(2, &poll_set, nullptr, nullptr, &timeout_def);
    HttpClient::receive_msg(conn,outbuf.size(), outbuf.data(), handler);

    ASSERT_TRUE(result);

    std::string response_start(buf_start.data(), expected_start.size());
    ASSERT_STREQ(expected_start.c_str(), response_start.c_str());
}

TEST(HttpClient, partial_download) {
std::string
        host_name("masters-thesis-mk.s3.eu-central-1.amazonaws.com"),
        public_object("/benchmark/public/largefile-1G.bin");

std::string headers = "GET " + public_object + " HTTP/1.1\r\n"
                                               "Host: " + host_name + "\r\n"
                                                                      "Range: bytes=0-2048         \r\n\r\n";

auto conn = HttpClient::create_connection(HttpClient::lookup_host(host_name));
auto conns = std::array{ conn };
struct timeval timeout_def{ 3, 0 };
auto poll_set = Connection::make_fd_set(conns);
select(2, nullptr, &poll_set, nullptr, &timeout_def);
HttpClient::send_msg(conn, headers);

std::string expected_start = "HTTP/1.1 206 Partial Content";
bool result = false;
std::vector<char> buf_start(expected_start.size());
std::vector<char> outbuf(1200);
auto handler = [&result, &buf_start](const Connection &conn, size_t received, char* buffer) {
    if (!result) {
        memcpy(buf_start.data(), buffer, buf_start.size());
    }
    result = true;
    sout << "----------------\nReceived data with recv_len " << received << "\n----------------" << std::endl;
    sout << std::string(buffer, received) << std::endl;
};

poll_set = Connection::make_fd_set(conns);
timeout_def.tv_sec = 3;
select(2, &poll_set, nullptr, nullptr, &timeout_def);
HttpClient::receive_msg(conn, outbuf.size(), outbuf.data(), handler);

ASSERT_TRUE(result);

std::string response_start(buf_start.data(), expected_start.size());
ASSERT_STREQ(expected_start.c_str(), response_start.c_str());
}

}  // namespace s3benchmark