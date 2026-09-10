# Zenbook Fan Control

A lightweight, complete stack (Kernel Module + C++ Daemon + CLI) for managing thermal policies and fan modes on ASUS Zenbook laptops. 

This project bypasses standard ACPI limitations by interacting directly with the ASUS WMI interface to toggle between Quiet, Balanced, and Performance modes.

## Architecture

The project consists of three decoupled layers:

1. **`zenbook_ec` (Kernel Module):** A custom DKMS-enabled driver that hooks into the ASUS WMI interface (`0x00110019`) and exposes a clean `sysfs` node at `/sys/devices/platform/zenbook_fan/fan_mode`.
2. **`zenbook-fan-daemon`:** A background C++20 daemon that securely manages hardware state and listens for incoming commands via a UNIX domain socket (`/run/zenbook_fan.sock`).
3. **`fan-cli`:** A fast, unprivileged command-line interface that communicates with the daemon to get, set, or toggle fan profiles.

## Features

* **Direct WMI Control:** Reliable thermal policy switching via kernel-level WMI calls.
* **Low Footprint:** The daemon sleeps natively on a UNIX socket until a command is received, consuming zero CPU cycles in the background.
* **Desktop Agnostic (Hyprland Ready):** Includes an automated script to inject keybinds directly into your Hyprland config (e.g., `Shift + Fn + F`).
* **Systemd Integrated:** Graceful startup, shutdown, and auto-restart capabilities via systemd.

## Prerequisites

* **OS:** Linux (Arch, Ubuntu, Fedora, etc.)
* **Compiler:** `g++` (with C++20 support) and `cmake` (>= 3.20)
* **Kernel headers:** Required for module compilation
* **DKMS:** Recommended for automatic module rebuilding on kernel updates

## Installation & Setup

The easiest way to install the entire stack is using the provided interactive shell script. It handles DKMS module registration, CMake builds, systemd service installation, and even Hyprland keybind injection

```bash
git clone https://github.com/Potato3852/Fan-control-Driver
cd Fan-control-Driver
chmod +x scripts/service.sh
./scripts/service.sh
```

## Manual Build
If you prefer to build **manually**:
```bash
# 1. Build & load kernel module
cd driver && make
sudo insmod build/zenbook_ec.ko

# 2. Build daemon & CLI
cd ..
cmake -B build -DCMAKE_CXX_COMPILER=g++
cmake --build build
sudo cmake --install build
```

## Usage
Once the daemon is running, use the `fan-cli` to interact with your laptop's thermal policy:
* **Get current mode:**
```bash
fan-cli get
```
* **Set specific mode (0 = Quiet, 1 = Balanced, 2 = Performance):**
```bash
fan-cli set 2
```
* **Toggle through modes sequentially (perfect for keybinds)**
```bash
fan-cli toggle
```

## Uninstallation
To remove all binaries, service, kernel modules, and clean up Hyprland configs, simply run:
```bash
./service.sh
# Select Option 2: "Uninstall service, CLI & kernel module"
```

---

<div align="center">

Built with pure enthusiasm (not love) by **Potato3852**  
*AI Mentor & Code Reviewer: Google Gemini*

</div>