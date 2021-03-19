//
// Created by Maximilian Kuschewski on 2021-03-18
//
#ifndef _BENCHMARK_IO_H_
#define _BENCHMARK_IO_H_

#include <unistd.h>
#include <sys/errno.h>

// file stack
#include <sys/fcntl.h>
// #include <sys/eventfd.h>

// network stack
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>  // addrinfo -> lookup
// #include <arpa/inet.h>

// async i/o with io_uring
#include <linux/io_uring.h>
#include <liburing.h>

#include <cstring>
#include <iostream>
#include <optional>

namespace benchmark {
    // ----------------------------------------------------------------------------------------------------
    struct IO {
        // ------------------------------------------------------------------------------------------------
        using sevent = struct io_uring_sqe;
        using cevent = struct io_uring_cqe;
        // ------------------------------------------------------------------------------------------------
        struct io_uring ring;
        // ------------------------------------------------------------------------------------------------
        IO(unsigned entries, int flags = 0) {
            int res = io_uring_queue_init(entries, &ring, flags);
            if (res != 0) {
                std::cerr << "error while initializing io_uring queue: " << errno << std::endl;
                throw std::system_error();
            }
        }
        // ------------------------------------------------------------------------------------------------
        ~IO() {
            io_uring_queue_exit(&ring);
        }
        // ------------------------------------------------------------------------------------------------
        void submit_queue() {
            io_uring_submit(&ring); // syscall
        }
        // ------------------------------------------------------------------------------------------------
        uint64_t peek_completion_data() {
            cevent* cqe;
            auto res = io_uring_peek_cqe(&ring, &cqe);
            if (res == 0) {
                auto ud = cqe->user_data;
                io_uring_cqe_seen(&ring, cqe);
                return ud;
            } else if (res < 0 && res != -EAGAIN) {
                std::cerr << "error " << -res << " while peeking iouring completion." << std::endl;
            }
            return 0;
        }
        // ------------------------------------------------------------------------------------------------
        template<typename CB>
        int peek_completion(CB callback) {
            cevent* cqe;
            auto res = io_uring_peek_cqe(&ring, &cqe);
            if (res == 0) {
                callback(cqe);
                io_uring_cqe_seen(&ring, cqe);
            } else if (res < 0 && res != -EAGAIN) {
                std::cerr << "error " << -res << " while peeking iouring completion." << std::endl;
            }
            return res;
        }
        // ------------------------------------------------------------------------------------------------
        std::pair<int, cevent*> wait_completion() {
            cevent* cqe;
            return std::make_pair(io_uring_wait_cqe(&ring, &cqe), cqe);
        }
        void mark_seen(cevent* event) {
            io_uring_cqe_seen(&ring, event);
        }
        // ------------------------------------------------------------------------------------------------
        template<bool advance = true>
        std::pair<int32_t, uint64_t> wait_completion_result() {
            cevent* cqe;
            auto res = io_uring_wait_cqe(&ring, &cqe);
            if (res == 0) {
                auto ud = cqe->user_data;
                auto code = cqe->res;
                if constexpr (advance) {
                        io_uring_cqe_seen(&ring, cqe);
                }
                return std::make_pair(code, ud);
            } else if (res < 0) {
                std::cerr << "error " << -res << " while waiting for iouring completion." << std::endl;
            }
            std::cerr << "Completion event was " << res << " and cqe->res was " << cqe->res;
            return { res, -1ul };
        }
        // ------------------------------------------------------------------------------------------------
        template<bool advance = true>
        uint64_t wait_completion_data() {
            cevent* cqe;
            auto res = io_uring_wait_cqe(&ring, &cqe);
            if (res == 0) {
                auto ud = cqe->user_data;
                if constexpr (advance) {
                        io_uring_cqe_seen(&ring, cqe);
                }
                return ud;
            } else if (res < 0) {
                std::cerr << "error " << -res << " while waiting for iouring completion." << std::endl;
            }
            std::cerr << "Completion event was " << res << " and cqe->res was " << cqe->res;
            return 0;
        }
        // ------------------------------------------------------------------------------------------------
        template<bool advance = true>
        int wait_for_n(unsigned n) {
            cevent* cqe;
            auto res = io_uring_wait_cqe_nr(&ring, &cqe, n);
            if (res == 0) {
                if constexpr (advance) {
                        io_uring_cq_advance(&ring, n);
                }
            } else if (res < 0) {
                std::cerr << "error " << -res << " while waiting for iouring completion." << std::endl;
            }
            return res;
        }
        // ------------------------------------------------------------------------------------------------
        template<typename CB>
        int wait_completion(CB callback) {
            cevent* cqe;
            auto res = io_uring_wait_cqe(&ring, &cqe);
            if (res == 0) {
                callback(cqe);
            } else if (res < 0) {
                std::cerr << "error " << -res << " while waiting for iouring completion." << std::endl;
            }
            io_uring_cqe_seen(&ring, cqe);
            return res;
        }
        // ------------------------------------------------------------------------------------------------
        unsigned ready_events() {
            return io_uring_cq_ready(&ring);
        }
        // ------------------------------------------------------------------------------------------------
    }; // struct IO
    // ----------------------------------------------------------------------------------------------------
    struct SocketIO : IO {
        // ------------------------------------------------------------------------------------------------
        SocketIO(unsigned entries, int flags = 0) : IO(entries, flags) {}
        // ------------------------------------------------------------------------------------------------
        static struct addrinfo lookup(const char* hostname, int port) {
            struct addrinfo hints;
            struct addrinfo *lookup;
            ::memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_NUMERICSERV;
            hints.ai_protocol = 0;
            std::string port_str = std::to_string(port);
            int ret = ::getaddrinfo(hostname, port_str.c_str(), &hints, &lookup);
            if (ret != 0) {
                std::cerr << "Error " << ret << "while looking up hostname " << hostname
                          << " with message " << ::gai_strerror(ret) << std::endl;
                throw std::runtime_error("error while looking up hostname");
            }
            if (lookup == nullptr) {
                std::cerr << "Address lookup succeeded, but no address returned." << std::endl;
                throw std::runtime_error("error while looking up hostname");
            }
            auto [fd, success] = connect(lookup);
            if (fd == -1) {
                std::cerr << "Could not establish a useable connection.";
                throw std::runtime_error("error while connecting");
            }
            struct sockaddr* addr = new struct sockaddr();
            struct addrinfo result;
            ::memcpy(addr, success->ai_addr, sizeof(struct sockaddr));
            ::memcpy(&result, success, sizeof(struct addrinfo));
            result.ai_addr = addr;
            freeaddrinfo(lookup);
            close(fd);
            return result;
        }
        // ------------------------------------------------------------------------------------------------
        static int connect(const char* hostname, int port = 80) {
            struct addrinfo hints;
            struct addrinfo *lookup;
            ::memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_NUMERICSERV;
            hints.ai_protocol = 0;
            std::string port_str = std::to_string(port);
            int ret = ::getaddrinfo(hostname, port_str.c_str(), &hints, &lookup);
            if (ret != 0) {
                std::cerr << "Error " << ret << "while looking up hostname " << hostname
                          << " with message " << ::gai_strerror(ret) << std::endl;
                return -1;
            }
            if (lookup == nullptr) {
                std::cerr << "Address lookup succeeded, but no address returned." << std::endl;
                return -1;
            }
            auto [fd, success] = connect(lookup);
            if (fd == -1) {
                std::cerr << "Could not establish a useable connection.";
            }
            freeaddrinfo(lookup);
            return fd;
        }
        // ------------------------------------------------------------------------------------------------
        static inline std::pair<int, struct addrinfo*> connect(struct addrinfo* lookup) {
            for (auto addr = lookup; addr != nullptr; addr = addr->ai_next) {
                if (addr->ai_family != AF_INET && addr->ai_family != AF_INET6) {
                    continue;
                }
                int fd = make_socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
                if (fd == -1) { continue; }
                if (0 == ::connect(fd, addr->ai_addr, addr->ai_addrlen)) {
                    return std::make_pair(fd, addr);
                }
                int err = errno;
                std::cerr << "Could not connect to " << addr->ai_family << "/" << addr->ai_socktype << "/" << addr->ai_protocol
                          << " with error " << err << std::endl;
                ::close(fd);
            }
            return std::make_pair(-1, nullptr);
        }
        // ------------------------------------------------------------------------------------------------
        template<bool submit = true>
        sevent* send_async(int fd, void* data, unsigned size, uint64_t id, int flags = 0) {
            auto sqe = io_uring_get_sqe(&ring);
            io_uring_prep_send(sqe, fd, data, size, flags);
            sqe->user_data = id;
            if constexpr (submit) {
                    io_uring_submit(&ring); // syscall
            }
            return sqe;
        }
        // ------------------------------------------------------------------------------------------------
        template<bool submit = true>
        sevent* recv_async(int fd, void* data, unsigned size, uint64_t id, int flags = 0) {
            auto sqe = io_uring_get_sqe(&ring);
            io_uring_prep_recv(sqe, fd, data, size, flags);
            sqe->user_data = id;
            if constexpr (submit) {
                    io_uring_submit(&ring); // syscall
            }
            return sqe;
        }
        // ------------------------------------------------------------------------------------------------
        static inline void close(int fd) {
            ::close(fd);
        }
        // ------------------------------------------------------------------------------------------------
    private:
        static inline int make_socket(int family = AF_INET, int type = SOCK_STREAM, int protocol = 0) {
          int fd = ::socket(AF_INET, SOCK_STREAM, 0);
          if (fd == -1) {
              auto err = errno;
              std::cerr << "Error " << err << " opening socket" << std::endl;
              if (err == 24) {
                  std::cerr << "seems to be b/c of too many open files try ulimit -n" << std::endl;
              }
              return -1;
          }
          int flags = 1;
          if (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags))) {
              auto err = errno;
              std::cerr << "Error " << err << " using SO_KEEPALIVE" << std::endl;
              throw std::runtime_error("Error using setsocketopt() for SO_KEEPALIVE");
          }
          flags = 120;
          if (::setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &flags, sizeof(flags))) {
              auto err = errno;
              std::cerr << "Error " << err << " using TCP_KEEPIDLE" << std::endl;
              throw std::runtime_error("Error using setsocketopt() for SO_KEEPIDLE");
          }
          // flags = 10;
          // if(::setsockopt(fd, SOL_TCP, TCP_INIT_CWND, &10, sizeof(flags))) {
          //     auto err = errno;
          //     std::cerr << "Error " << err << " using TCP_INIT_CWND" << std::endl;
          //     throw std::runtime_error("Error using setsocketopt() for SO_KEEPIDLE");
          // }
          return fd;
        }
        // ------------------------------------------------------------------------------------------------
    };
    // ----------------------------------------------------------------------------------------------------
    struct DiskIO : IO {
        // ------------------------------------------------------------------------------------------------
        DiskIO(unsigned entries, int flags = 0) : IO(entries, flags) {}
        // ------------------------------------------------------------------------------------------------
        static int open_write(const char* filename) {
            return open(filename, O_DIRECT | O_WRONLY | O_CREAT, 0777);
        }
        // ------------------------------------------------------------------------------------------------
        static int open_read(const char* filename) {
            return open(filename, O_DIRECT | O_RDONLY);
        }
        // ------------------------------------------------------------------------------------------------
        static int open_rw(const char* filename) {
            return open(filename, O_DIRECT | O_RDWR | O_CREAT, 0777);
        }
        // ------------------------------------------------------------------------------------------------
        static int close(int fd) {
            return ::close(fd);
        }
        // ------------------------------------------------------------------------------------------------
        static int delete_file(const char* filename) {
            return ::remove(filename);
        }
        // ------------------------------------------------------------------------------------------------
        template<bool submit = true>
        sevent* fallocate_async(int fd, unsigned size, uint64_t id = 0, unsigned offset = 0) {
            auto sqe = io_uring_get_sqe(&ring);
            io_uring_prep_fallocate(sqe, fd, 0, offset, size);
            sqe->user_data = id;
            if constexpr (submit) {
                    io_uring_submit(&ring); // syscall
            }
            return sqe;
        }
        // ------------------------------------------------------------------------------------------------
        template<bool submit = true>
        sevent* write_async(int fd, void* data, unsigned size, uint64_t id = 0, unsigned offset = 0) {
            auto sqe = io_uring_get_sqe(&ring);
            io_uring_prep_write(sqe, fd, data, size, offset);
            sqe->user_data = id;
            if constexpr (submit) {
                    io_uring_submit(&ring); // syscall
            }
            return sqe;
        }
        // ------------------------------------------------------------------------------------------------
        template<bool submit = true>
        sevent* fsync_async(int fd, uint64_t id = 0, bool datasync = true) {
            auto sqe = io_uring_get_sqe(&ring);
            io_uring_prep_fsync(sqe, fd, datasync ? IORING_FSYNC_DATASYNC : 0);
            sqe->user_data = id;
            if constexpr (submit) {
                    io_uring_submit(&ring);
            }
            return sqe;
        }
        // ------------------------------------------------------------------------------------------------
        template<bool submit = true>
        sevent* read_async(int fd, void* data, unsigned size, uint64_t id = 0, unsigned offset = 0) {
            auto sqe = io_uring_get_sqe(&ring);
            io_uring_prep_read(sqe, fd, data, size, offset);
            sqe->user_data = id;
            if constexpr (submit) {
                    io_uring_submit(&ring); // syscall
            }
            return sqe;
        }
        // ------------------------------------------------------------------------------------------------
    };  // struct DiskIO
    // ----------------------------------------------------------------------------------------------------
    template<unsigned LENGTH, unsigned PREFIX, typename dynamic_t = uint64_t>
    union FastFilename {
        static_assert(PREFIX + sizeof(dynamic_t) <= LENGTH, "Overall string length must be larger than dynamic part plus prefix!");
        char string[LENGTH];
        struct {
            char prefix[PREFIX];
            union {
                dynamic_t value;
                char chars[sizeof(dynamic_t)];
            } dynamic;
        } __attribute__((packed));
    };  // union FastFilename
    // ----------------------------------------------------------------------------------------------------
}  // namespace benchmark

#endif // _BENCHMARK_IO_H_
