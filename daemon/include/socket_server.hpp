#pragma once

#include "fan_controller.hpp"
#include <string>
#include <string_view>
#include <format>
#include <filesystem>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace fs = std::filesystem;

namespace zenbook::ipc {
    
template <typename Controller>
std::string process_command(std::string_view req, Controller& controller) {
    try {
        if (req.starts_with("GET")) {
            int mode = controller.get_current_mode();
            return std::format("{}\n", mode);
        }
        
        if (req.starts_with("SET ")) {
            if (req.size() < 5) {
                return "ERR: Missing mode argument\n";
            }
        
            int mode = req[4] - '0';
            if (mode < 0 || mode > 2) {
                return std::format("ERR: Invalid mode: {}\n", mode);
            }
        
            controller.set_mode(mode);
            return std::format("OK: Switched to mode {}\n", mode);
        }
        
        if (req.starts_with("TOGGLE")) {
            int current = controller.get_current_mode();
            int next = (current + 1) % 3;
            controller.set_mode(next);
            return std::format("OK: Toggled to mode {}\n", next);
        }

    } catch (const std::exception& e) {
        return std::format("ERR: Hardware error: {}\n", e.what());
    }
    
    return "ERR: Unknown command\n";
}

template <typename Controller>
class SocketServer {
private:
    Controller& controller_;
    fs::path socket_path_;
    int server_fd_{-1};
    std::jthread worker_;

    void init_socket() {
        if (fs::exists(socket_path_)) {
            fs::remove(socket_path_);
        }

        server_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd_ == -1) {
            throw std::runtime_error("Failed to create UNIX domain socket");
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

        if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
            ::close(server_fd_);
            throw std::runtime_error(std::format("Failed to bind socket to path: {}", socket_path_.string()));
        }

        if (::listen(server_fd_, 5) == -1) {
            ::close(server_fd_);
            throw std::runtime_error("Failed to listen on UNIX domain socket");
        }
    }

    void run(std::stop_token stop_token) {
        timeval tv{.tv_sec = 0, .tv_usec = 500'000};
        ::setsockopt(server_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        while (!stop_token.stop_requested()) {
            int client_fd = ::accept(server_fd_, nullptr, nullptr);
            if (client_fd == -1) {
                continue;
            }

            char buffer[128]{0};
            ssize_t bytes_read = ::read(client_fd, buffer, sizeof(buffer) - 1);
            
            if (bytes_read > 0) {
                std::string_view request(buffer, bytes_read);
                std::string response = process_command(request, controller_);
                
                ::write(client_fd, response.data(), response.size());
            }

            ::close(client_fd);
        }
    }

public:
    SocketServer(Controller& controller, fs::path socket_path = "/run/zenbook_fan.sock") : controller_(controller), socket_path_(std::move(socket_path)) {
        init_socket();
        worker_ = std::jthread([this](std::stop_token st) { run(st); });
    }

    ~SocketServer() {
        if (server_fd_ != -1) {
            ::close(server_fd_);
        }
        if (fs::exists(socket_path_)) {
            fs::remove(socket_path_);
        }
    }
};

} // namespace zenbook::ipc