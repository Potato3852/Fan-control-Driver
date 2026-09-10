#pragma once

#include <concepts>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <format>

namespace fs = std::filesystem;

/**
 * @brief Concept defining the hardware backend interface for fan control
 * @tparam T Hardware backend type implementing read and write operations.
 */
template <typename T>
concept FanHardwareInterface = requires(const T const_hw, T hw, int mode) {
    { const_hw.read_profile() } -> std::same_as<int>;
    { hw.write_profile(mode)  } -> std::same_as<void>;
};

/**
 * @brief Hardware backend interacting with the fan driver via sysfs
 */
class SysfsFanHardware {
private:
    fs::path sysfs_path_;

public:
    /**
     * @brief Construct a sysfs fardware interface.
     * @param path Path to the fan_mode file in /sys
     * @throws std::runtime_error If the specified path does not exist
     */
    explicit SysfsFanHardware(fs::path path = "/sys/devices/platform/zenbook_fan/fan_mode");

    /**
     * @brief Read current fan profile from sysfs
     * @return Profile mode (0, 1, 2)
     */
    [[nodiscard]] int read_profile() const;

    /**
     * @brief Write a new fan profile mode to sysfs.
     * @param mode Taget profile mode (0, 1, 2)
     */
    void write_profile(int mode);
};

/**
 * @brief High-level controller managing fan cooling profiles
 * @tparam HardwareBackend Backend implementation (defaults to SysfsFanHardware).
 */
template <FanHardwareInterface HardwareBackend = SysfsFanHardware>
class FanController {
private:
    HardwareBackend backend_;

public:
    explicit FanController(HardwareBackend backend = HardwareBackend{}) : backend_(std::move(backend)) {}

    [[nodiscard]] int get_current_mode() const { return backend_.read_profile(); }
    void set_mode(int mode) { backend_.write_profile(mode); }
};