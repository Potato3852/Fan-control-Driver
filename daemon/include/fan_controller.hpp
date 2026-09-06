#pragma once

#include <concepts>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <format>

namespace fs = std::filesystem;

template <typename T>
concept FanHardwareInterface = requires(const T const_hw, T hw, int mode) {
    { const_hw.read_profile() } -> std::same_as<int>;
    { hw.write_profile(mode)  } -> std::same_as<void>;
};



class SysfsFanHardware {
private:
    fs::path sysfs_path_;

public:
    explicit SysfsFanHardware(fs::path path = "/sys/devices/platform/zenbook_fan/fan_mode");

    [[nodiscard]] int read_profile() const;
    void write_profile(int mode);
};



template <FanHardwareInterface HardwareBackend = SysfsFanHardware>
class FanController {
private:
    HardwareBackend backend_;

public:
    explicit FanController(HardwareBackend backend = HardwareBackend{}) : backend_(std::move(backend)) {}

    [[nodiscard]] int get_current_mode() const { return backend_.read_profile(); }
    void set_mode(int mode) { backend_.write_profile(mode); }
};