#include "fan_controller.hpp"

SysfsFanHardware::SysfsFanHardware(fs::path path) 
    : sysfs_path_(std::move(path)) {
    if (!fs::exists(sysfs_path_)) {
        throw std::runtime_error(std::format("Sysfs path does not exist: {}", sysfs_path_.string()));
    }
}

int SysfsFanHardware::read_profile() const {
    std::ifstream file(sysfs_path_);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open sysfs file: {}", sysfs_path_.string()));
    }

    int mode = -1;
    file >> mode;

    if (file.fail()) {
        throw std::runtime_error("Failed to parse profile value from sysfs");
    }

    return mode;
}

void SysfsFanHardware::write_profile(int mode) {
    if (mode < 0 || mode > 2) {
        throw std::invalid_argument(std::format("Invalid fan profile mode: {}", mode));
    }

    std::ofstream file(sysfs_path_);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open sysfs file: {}", sysfs_path_.string()));
    }

    file << mode;
    if (file.fail()) {
        throw std::runtime_error(std::format("Failed to apply profile {} to sysfs", mode));
    }
}