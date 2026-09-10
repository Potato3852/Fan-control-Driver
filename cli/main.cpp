#include <iostream>
#include <string>
#include <string_view>
#include <format>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Helper function for detecting the on/off state of notifications.
 * @return 1 - notifications enabled, 0 - notification disabled
 */
bool is_notify_enabled_in_config() {
    const char* home = std::getenv("HOME");
    if (!home) return false;
    fs::path config_path = fs::path(home) / ".config" / "zenbook-fan" / "notify";
    return fs::exists(config_path);
}

int main(int argc, char* argv[]) {
    // Verification of the correctness of the arguments
    if (argc < 2) {
        std::cerr << "Usage: fan-cli [-n|--notify] <get|set|toggle> [mode]\n";
        return 1;
    }

    bool enable_notify = is_notify_enabled_in_config();
    int arg_offset = 1;

    if (std::string_view(argv[1]) == "-n" || std::string_view(argv[1]) == "--notify") {
        enable_notify = true;
        arg_offset++;
        if (argc < 3) {
            std::cerr << "Usage: fan-cli [-n|--notify] <get|set|toggle> [mode]\n";
            return 1;
        }
    }

    std::string_view command = argv[arg_offset];
    std::string request;

    // Determining the type of command
    if (command == "get") {
        request = "GET";
    } else if (command == "set") {
        if (argc < arg_offset + 2) {
            std::cerr << "Error: 'set' requires a mode argument (0, 1, or 2)\n";
            return 1;
        }
        request = std::format("SET {}", argv[arg_offset + 1]);
    } else if (command == "toggle") {
        request = "TOGGLE";
    } else {
        std::cerr << "Error: Unknown command. Use get, set, or toggle.\n";
        return 1;
    }

    // Create socket between the CLI and the daemon.
    int client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);

    if (client_fd == -1) {
        std::cerr << "Error: create socket failed.\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, "/run/zenbook_fan.sock", sizeof(addr.sun_path) - 1);

    // Connect to daemon
    if (::connect(client_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::cerr << "Error: connection to daemon failed.\n";
        ::close(client_fd);
        return 1;
    }

    ::write(client_fd, request.data(), request.size());

    char buffer[128]{0};
    ssize_t bytes_read = ::read(client_fd, buffer, sizeof(buffer) - 1);

    // Determining the response
    if (bytes_read > 0) {
        std::string response(buffer, bytes_read);
        std::cout << response;

        if (enable_notify) {
            if (!response.empty() && response.back() == '\n') {
                response.pop_back();
            }
            std::string notify_cmd = std::format(
                "notify-send -u low -i fan -t 2000 \"Thermal Profile\" \"{}\" 2>/dev/null", 
                response
            );
            ::system(notify_cmd.c_str());
        }
    }

    // Close connection
    ::close(client_fd);
    return 0;
}