//
// Created by Maximilian Kuschewski on 13.06.20.
//
#include "s3benchmark/HttpClient.hpp"
#include "s3benchmark/Config.hpp"
// Sockets & network
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <errno.h>
// C++ std
#include <iostream>
#include <vector>
#include <cstring>

namespace s3benchmark {

    const size_t STR_PADDING_SIZE = 5;
    HttpClient::HttpClient(const std::string &initial_request_headers,
                           size_t dynamic_header_index,
                           const response_handler_t &response_handler)
            : request_header_size(initial_request_headers.size())
            , request_headers(new char[request_header_size + STR_PADDING_SIZE])
            , dynamic_header_index(dynamic_header_index)
            , response_handler(response_handler)
            , socket_descriptor(-1) {
        std::copy(initial_request_headers.begin(), initial_request_headers.end(), this->request_headers);
        auto end = "\r\n\r\n\0";
        for (int i = 0; i < STR_PADDING_SIZE; ++i) {
            this->request_headers[request_header_size + i] = end[i];  // C string end
        }
    }

    HttpClient::~HttpClient() {
        close_connection();
        delete[] this->request_headers;
    };

    char* HttpClient::dynamic_header() const {
        return this->request_headers + dynamic_header_index;
    }

    size_t HttpClient::header_size() const {
        return this->request_header_size;
    }

    size_t HttpClient::dynamic_header_size() const {
        return this->request_header_size - this->dynamic_header_index;
    }

    void HttpClient::open_connection(const hostent* host) {
        using sock_addr = struct sockaddr;
        using inet_addr = struct sockaddr_in;
        // Initialize socket and address struct
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        inet_addr destination;
        memset(&destination, 0, sizeof(inet_addr));
        // Set required address fields
        destination.sin_family = AF_INET;
        destination.sin_port = htons(80);
        destination.sin_addr.s_addr = *((unsigned long *) host->h_addr);
        // std::cout << "Trying to connect to ip " << inet_ntoa(destination.sin_addr) << std::endl;
        // Connect to the socket, throw if not possible
        int is_connected = connect(fd, (sock_addr *) &destination, sizeof(sock_addr));
        if (is_connected == -1) {
            close(fd);
            throw std::runtime_error("Could not connect socket to host " + std::string(host->h_name));
        }
        // int tcp_nodelay_opt = 1;
        // if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay_opt, sizeof(tcp_nodelay_opt)) < 0) {
        //     throw std::runtime_error("Error using setsockopt() for TCP_NODELAY.");
        // }
        this->socket_descriptor = fd;
    }

    void HttpClient::execute_request(const size_t &buffer_size, char* recv_buffer) const {
        if (socket_descriptor == -1) {
            throw std::runtime_error("Call open_connection to initialize the connection first.");
        }
        // TODO: use send fn w/ flags arg instead, search for flags w/ best efficiency
        // std::cout << "Sending the following request on socket " << this->socket_descriptor << std::endl;
        // std::cout << this->request_headers << std::endl;
        send(this->socket_descriptor, this->request_headers, this->request_header_size, 0);
       //  while(true) {
        auto poll_def = pollfd{
                this->socket_descriptor,
                POLLIN | POLLMSG | POLLNVAL,
                0
        };
        poll:
        auto poll_res = poll(&poll_def, 1, Config::REQUEST_TIMEOUT_MS);
        if (poll_res == 0) {
            std::cerr << "Timeout while polling on socket " << this->socket_descriptor
                      << " with events " << poll_def.revents
                      << std::endl;
            return;
        } else if (poll_res < 0) {
            std::cerr << "An error occured while polling for the socket " << this->socket_descriptor
                      << " with events " << poll_def.revents << std::endl;
            return;
        }
        if ((poll_def.revents & POLLNVAL) || (poll_def.revents & POLLHUP) || (poll_def.revents & POLLERR)) {
            std::cerr << "Socket hung up or closed with fd " << this->socket_descriptor
                      << " with events " << poll_def.revents
                      << std::endl;
            return;
        }
        if (poll_def.revents & POLLIN) {
            // std::cout << "Socket has data to read, continuing to read on fd " << this->socket_descriptor
            //           << " with events " << poll_def.revents << std::endl;
            unsigned recv_len, recv_sum = 0;
            do {
                recv_len = recv(this->socket_descriptor, recv_buffer, buffer_size, MSG_DONTWAIT);
                if (recv_len == -1) {
                    auto read_err = errno;
                    if (read_err == EWOULDBLOCK) {
                        goto poll;
                    }
                    std::cerr << "Read error: " << read_err << std::endl;
                    break;
                } else if (recv_len == 0) {
                    // std::cout << "No more data to receive." << std::endl;
                    break;
                } else {
                    this->response_handler(recv_len, recv_buffer);
                    if(recv_sum == 0) {
                        std::cout << "\n-------------------Received data start " << recv_len << ":----------------------" << std::endl;
                        std::cout << "Sent the following request on socket " << this->socket_descriptor << std::endl;
                        std::cout << this->request_headers << std::endl;
                        std::cout << "\n--------------------------------------------------------------" << std::endl;
                        std::cout << recv_buffer << std::endl;
                        std::cout << "\n--------------------------------------------------------------" << std::endl;
                        std::cout << "\n--------------------------------------------------------------" << std::endl;
                    }
                    std::cout << "recv_len " << recv_len << std::endl;
                    recv_sum += recv_len;
                }
            } while(recv_len > 0); // Request done if less than buffer size was read
        } else {
            std::cerr << "Unknown events received from socket " << poll_def.revents << std::endl;
        // }
        }
    }

    void HttpClient::close_connection() {
        if (socket_descriptor != -1) {
            // std::cout << "Closing connection for socket " << socket_descriptor << std::endl;
            close(socket_descriptor);
        }
    }

    hostent* HttpClient::lookup_host(const std::string &url) {
        return gethostbyname(url.c_str()); // TODO: DNS cache?
    }
}  // namespace s3benchmark

