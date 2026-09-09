#include <iostream>
#include <string>
#include <string_view>
#include <format>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: fan-cli <get|set|toggle> [mode]\n";
        return 1;
    }

    std::string_view command = argv[1];
    std::string request;

    if (command == "get") {
        request = "GET";
    } else if (command == "set") {
        if (argc < 3) {
            std::cerr << "Error: 'set' requires a mode argument (0, 1, or 2)\n";
            return 1;
        }
        request = std::format("SET {}", argv[2]);
    } else if (command == "toggle") {
        request = "TOGGLE";
    } else {
        std::cerr << "Error: Unknown command. Use get, set, or toggle.\n";
        return 1;
    }

    int client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);

    if (client_fd == -1) {
        std::cerr << "Error: create socket failed.\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, "/run/zenbook_fan.sock", sizeof(addr.sun_path) - 1);

    if(::connect(client_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::cerr << "Error: connection to daemon failed.\n";
        return 1;
    }

    ::write(client_fd, request.data(), request.size());

    char buffer[128]{0};
    ssize_t bytes_read = ::read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        std::cout << buffer;
    }

    ::close(client_fd);

    return 0;
}