#include "fan_controller.hpp"
#include "socket_server.hpp"
#include <iostream>
#include <csignal>
#include <condition_variable>
#include <mutex>

namespace {
    std::condition_variable shutdown_cv;
    std::mutex shutdown_mutex;
    bool shutdown_requested = false;

    void handle_signal(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            std::lock_guard<std::mutex> lock(shutdown_mutex);
            shutdown_requested = true;
            shutdown_cv.notify_one();
        }
    }
}

int main() {
    try {
        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        SysfsFanHardware hardware;
        FanController controller(hardware);
        zenbook::ipc::SocketServer server(controller);

        std::cout << "[zenbook-fan-daemon] Service started.\n";

        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait(lock, [] { return shutdown_requested; });

        std::cout << "[zenbook-fan-daemon] Shutting down...\n";
    } 
    catch (const std::exception& e) {
        std::cerr << "[zenbook-fan-daemon] Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}