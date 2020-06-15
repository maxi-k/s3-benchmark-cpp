//
// Created by maxi on 15.06.20.
//
#include <s3benchmark/HttpClient.hpp>
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
                          "Host: example.com\r\n";
    std::string expected_start = "HTTP/1.1 200 OK";
    bool result = false;
    std::vector<char> buf_start(expected_start.size());
    HttpClient client(
            headers,
            headers.length(),
            [&result, &buf_start](size_t received, char* buffer) {
                if (!result) {
                    memcpy(buf_start.data(), buffer, buf_start.size());
                }
                result = true;
                sout << "\n----------------\nReceived data with recv_len " << received << "\n----------------\n" << std::endl;
                sout << buffer << std::endl;
            });

    std::vector<char> outbuf(500);
    client.open_connection(HttpClient::lookup_host("example.com"));
    client.execute_request(outbuf.size(), outbuf.data());
    std::flush(std::cout);

    ASSERT_TRUE(result);

    std::string response_start(buf_start.data(), expected_start.size());
    ASSERT_STREQ(expected_start.c_str(), response_start.c_str());
}

TEST(HttpClient, with_range_and_whitespace)
/// Test fetching from example.com
{
    std::string headers = "GET / HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Range: bytes=0-512\r\n";
    std::string expected_start = "HTTP/1.1 206 Partial Content";
    bool result = false;
    std::vector<char> buf_start(expected_start.size());
    HttpClient client(
            headers,
            headers.length(),
            [&result, &buf_start](size_t received, char* buffer) {
                if (!result) {
                    memcpy(buf_start.data(), buffer, buf_start.size());
                }
                result = true;
                sout << "\n----------------\nReceived data with recv_len " << received << "\n----------------\n" << std::endl;
                sout << buffer << std::endl;
            });

    std::vector<char> outbuf(500);
    client.open_connection(HttpClient::lookup_host("example.com"));
    client.execute_request(outbuf.size(), outbuf.data());
    std::flush(std::cout);

    ASSERT_TRUE(result);

    std::string response_start(buf_start.data(), expected_start.size());
    ASSERT_STREQ(expected_start.c_str(), response_start.c_str());
}

}  // namespace s3benchmark