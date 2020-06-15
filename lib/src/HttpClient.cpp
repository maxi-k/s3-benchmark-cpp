//
// Created by Maximilian Kuschewski on 13.06.20.
//
#include "s3benchmark/HttpClient.hpp"
// Sockets & network
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
// C++ std
#include <iostream>
#include <vector>

namespace s3benchmark {


    HttpClient::HttpClient(const std::string &initial_request_headers,
                           size_t dynamic_header_index,
                           const response_handler_t &response_handler)
                           : request_header_size(initial_request_headers.size())
                           , request_headers(new char[request_header_size + 1])
                           , dynamic_header_index(dynamic_header_index)
                           , response_handler(response_handler)
                           , socket_descriptor(-1) {
        std::copy(initial_request_headers.begin(), initial_request_headers.end(), this->request_headers);
        this->request_headers[request_header_size] = 0;  // C string end
    }

    HttpClient::~HttpClient() {
        close_connection();
        delete[] this->request_headers;
    };

    char* HttpClient::dynamic_header() const {
        return this->request_headers + dynamic_header_index;
    }

    inline size_t HttpClient::header_size() const {
        return this->request_header_size;
    }

    void HttpClient::open_connection(const std::string &url) {
        using sock_addr = struct sockaddr;
        using inet_addr = struct sockaddr_in;
        // Initialize socket and address struct
        int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        inet_addr destination;
        memset(&destination, 0, sizeof(inet_addr));
        // Set required address fields
        auto host = gethostbyname(url.c_str()); // TODO: DNS cache
        destination.sin_family = AF_INET;
        destination.sin_port = htons(80);
        destination.sin_addr.s_addr = *host->h_addr_list[0];
        // Connect to the socket, throw if not possible
        int is_connected = connect(fd, (sock_addr*)&destination, sizeof(sock_addr));
        if (is_connected == -1) {
            close(fd);
            throw std::runtime_error("Could not connect socket to url " + url);
        }
        this->socket_descriptor = fd;
    }

    void HttpClient::execute_request(const size_t &excpected_payload_length) const {
        if (socket_descriptor == -1) {
            throw std::runtime_error("Call open_connection to initialize the connection first.");
        }
        // TODO: use send fn w/ flags arg instead, search for flags w/ best efficiency
        write(this->socket_descriptor, this->request_headers, this->request_header_size);
        // Receive data
        unsigned recv_len;
        std::vector<char> buf(recv_len);
        // TODO use poll / ppoll / epoll / select (for multiple!)
        while ((recv_len = recv(this->socket_descriptor, buf.data(), excpected_payload_length, 0)) > 0) {
            std::cout << "Received data with recv_len " << recv_len << std::endl; // TODO
        }
    }

    void HttpClient::close_connection() {
        if (socket_descriptor != -1) {
            std::cout << "Closing connection for socket " << socket_descriptor << std::endl;
            close(socket_descriptor);
        }
    }

}  // namespace s3benchmark

