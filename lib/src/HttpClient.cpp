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

    ReadStats HttpClient::receive_msg(Connection &conn, const size_t &buffer_size, char* recv_buffer, const response_handler_t &handler) {
        if (!conn.is_open()) {
            throw std::runtime_error("Call open_connection to initialize the connection first.");
        }
        // std::cout << "Socket has data to read, continuing to read on fd " << this->socket_descriptor
        //           << " with events " << poll_def.revents << std::endl;
        int recv_len = 0;
        size_t recv_sum = 0, chunk_count = 0;
        auto read_size = buffer_size;
        auto t_start = clock::now();
        do {
            recv_len = recv(conn.socket, recv_buffer, read_size, MSG_DONTWAIT);
            if (recv_len == -1) {
                auto read_err = errno;
                if (read_err == EWOULDBLOCK && read_size > 100) {  // back-off strategy if socket would block
                    read_size /= 2;
                    continue;
                }
                std::cerr << "Read error: " << read_err << " on socket " << conn.socket << std::endl;
                break;
            } else if (recv_len == 0) {
                //std::cout << "No more data to receive." << std::endl;
                if (recv_sum == 0) {
                    // std::cerr << "Did not receive any data from request on socket " << conn.socket << std::endl;
                    // break;
                    throw std::runtime_error("Did not receive any data from request on socket.");
                }
                break;
            } else {
                recv_sum += recv_len;
                ++chunk_count;
                handler(conn, recv_len, recv_buffer);
            }
        } while(recv_len > 0);
        auto t_end = (conn.last_read = clock::now());
        return ReadStats{
            chunk_count,
            recv_sum,
            std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start)
        };
    }

    void HttpClient::send_msg(Connection &conn, const std::string &msg) {
        if (!conn.is_open()) {
            throw std::runtime_error("Call open_connection to initialize the connection first.");
        }
        conn.last_write = clock::now();
        send(conn.socket, msg.c_str(), msg.length(), 0); // TODO: search for flags w/ best efficiency
    }

    hostent* HttpClient::lookup_host(const std::string &url) {
        return gethostbyname(url.c_str()); // TODO: DNS cache?
    }

    Connection HttpClient::create_connection(const hostent* host) {
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
        // Set relevant socket options
        int flags =1;
        if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags))) {
            throw std::runtime_error("Error using setsocketopt() for SO_KEEPALIVE");
        }

        flags = 10;
        if (setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &flags, sizeof(flags))) {
            throw std::runtime_error("Error using setsocketopt() for TCP_KEEPIDLE");
        }
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
        return Connection{
            fd,
            clock::time_point::min(),
            clock::time_point::min()
        };
    }
}  // namespace s3benchmark

