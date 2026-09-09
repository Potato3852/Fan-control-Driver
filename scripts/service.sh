#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

CONFIG_DIR="$HOME/.config/zenbook-fan"
NOTIFY_FILE="$CONFIG_DIR/notify"
CUSTOM_KEYBINDS_DIR="$HOME/.config/hypr/custom"
CUSTOM_KEYBINDS_FILE="$CUSTOM_KEYBINDS_DIR/keybinds.conf"
SYSFS_PATH="/sys/devices/platform/zenbook_fan/fan_mode"
MODULE_NAME="zenbook_ec"
MODULE_VERSION="0.1.1"

find_keybinds_file() {
    if [ -f "$HOME/.config/hypr/custom/keybinds.conf" ]; then
        echo "$HOME/.config/hypr/custom/keybinds.conf"
    elif [ -f "$HOME/.config/hypr/hyprland/keybinds.conf" ]; then
        echo "$HOME/.config/hypr/hyprland/keybinds.conf"
    elif [ -f "$HOME/.config/hypr/hyprland.conf" ]; then
        echo "$HOME/.config/hypr/hyprland.conf"
    else
        echo ""
    fi
}

clean_hyprland_keybinds() {
    sed -i '/fan-cli toggle/d' "$HOME/.config/hypr/hyprland.conf" 2>/dev/null || true
    sed -i '/fan-cli toggle/d' "$HOME/.config/hypr/hyprland/keybinds.conf" 2>/dev/null || true
    sed -i '/fan-cli toggle/d' "$HOME/.config/hypr/custom/keybinds.conf" 2>/dev/null || true
    hyprctl reload 2>/dev/null || true
}

show_menu() {
    echo "=========================================="
    echo "    Zenbook Thermal Control Manager       "
    echo "=========================================="
    echo "1) Install kernel module (DKMS), service & CLI"
    echo "2) Uninstall service, CLI & kernel module"
    echo "3) Toggle Desktop Notifications"
    echo "4) Add Hyprland Keybind (Fn + Shift + F)"
    echo "5) Check Daemon Status"
    echo "6) View System Logs (journalctl)"
    echo "0) Exit"
    echo "=========================================="
}

install_service() {
    echo "[1/4] Installing kernel module..."
    if command -v dkms &> /dev/null; then
        echo "[INFO] DKMS detected. Registering module..."
        sudo dkms add ./driver || true
        sudo dkms build "${MODULE_NAME}/${MODULE_VERSION}" || true
        sudo dkms install "${MODULE_NAME}/${MODULE_VERSION}" --force
        
        echo "${MODULE_NAME}" | sudo tee /etc/modules-load.d/zenbook_ec.conf > /dev/null
        sudo modprobe "${MODULE_NAME}" || true
    else
        echo "[WARN] DKMS not found. Falling back to manual build..."
        make -C driver
        if [ ! -f "$SYSFS_PATH" ]; then
            sudo insmod driver/build/zenbook_ec.ko || true
        fi
    fi

    if [ ! -f "$SYSFS_PATH" ]; then
        echo "[ERROR] Failed to load kernel module. $SYSFS_PATH is missing."
        return 1
    fi

    echo "[2/4] Building C++ daemon and CLI..."
    cmake -B build -DCMAKE_CXX_COMPILER=g++
    cmake --build build

    echo "[3/4] Installing binaries and systemd service..."
    sudo cmake --install build

    echo "[4/4] Enabling and starting systemd service..."
    sudo systemctl daemon-reload
    sudo systemctl enable --now zenbook-fan

    echo "[INFO] Waiting for socket initialization..."
    for i in {1..30}; do
        if [ -S /run/zenbook_fan.sock ]; then
            break
        fi
        sleep 0.1
    done

    echo "[SUCCESS] Installation finished! Current mode:"
    fan-cli get || true
}

uninstall_service() {
    echo "[INFO] Stopping and disabling systemd service..."
    sudo systemctl disable --now zenbook-fan 2>/dev/null || true

    echo "[INFO] Unloading kernel module..."
    sudo rmmod "${MODULE_NAME}" 2>/dev/null || true

    if command -v dkms &> /dev/null; then
        echo "[INFO] Removing module from DKMS..."
        sudo dkms remove "${MODULE_NAME}/${MODULE_VERSION}" --all 2>/dev/null || true
    fi
    sudo rm -f /etc/modules-load.d/zenbook_ec.conf

    echo "[INFO] Cleaning up Hyprland keybinds..."
    clean_hyprland_keybinds

    echo "[INFO] Removing installed files..."
    sudo rm -f /usr/local/bin/zenbook-fan-daemon
    sudo rm -f /usr/local/bin/fan-cli
    sudo rm -f /etc/systemd/system/zenbook-fan.service
    sudo rm -f /run/zenbook_fan.sock

    sudo systemctl daemon-reload
    echo "[SUCCESS] Uninstallation complete!"
}

toggle_notifications() {
    mkdir -p "$CONFIG_DIR"
    if [ -f "$NOTIFY_FILE" ]; then
        rm -f "$NOTIFY_FILE"
        echo "[INFO] Desktop notifications DISABLED."
    else
        touch "$NOTIFY_FILE"
        echo "[INFO] Desktop notifications ENABLED."
    fi
}

add_hyprland_keybind() {
    KEYBINDS_FILE=""
    if [ -f "$HOME/.config/hypr/custom/keybinds.conf" ]; then
        KEYBINDS_FILE="$HOME/.config/hypr/custom/keybinds.conf"
    elif [ -f "$HOME/.config/hypr/hyprland/keybinds.conf" ]; then
        KEYBINDS_FILE="$HOME/.config/hypr/hyprland/keybinds.conf"
    elif [ -f "$HOME/.config/hypr/hyprland.conf" ]; then
        KEYBINDS_FILE="$HOME/.config/hypr/hyprland.conf"
    fi

    if [ -z "$KEYBINDS_FILE" ]; then
        echo "[ERROR] Could not locate Hyprland keybinds file."
        return 1
    fi

    sed -i '/fan-cli toggle/d' "$HOME/.config/hypr/hyprland.conf" 2>/dev/null || true
    sed -i '/fan-cli toggle/d' "$HOME/.config/hypr/hyprland/keybinds.conf" 2>/dev/null || true
    sed -i '/fan-cli toggle/d' "$HOME/.config/hypr/custom/keybinds.conf" 2>/dev/null || true

    BIND_CMD="bind = Shift, XF86Fn_F, exec, /usr/local/bin/fan-cli toggle"
    
    echo "" >> "$KEYBINDS_FILE"
    echo "# Zenbook Fan Control" >> "$KEYBINDS_FILE"
    echo "$BIND_CMD" >> "$KEYBINDS_FILE"
    
    echo "[SUCCESS] Added Fn+F keybind (XF86Fn_F) to $KEYBINDS_FILE"
    hyprctl reload 2>/dev/null || true
}

while true; do
    show_menu
    read -p "Select an option [0-6]: " choice
    echo ""
    case $choice in
        1) install_service ;;
        2) uninstall_service ;;
        3) toggle_notifications ;;
        4) add_hyprland_keybind ;;
        5) sudo systemctl status zenbook-fan --no-pager ;;
        6) sudo journalctl -u zenbook-fan -n 50 --no-pager ;;
        0) echo "Goodbye!"; exit 0 ;;
        *) echo "[ERROR] Invalid option. Please try again." ;;
    esac
    echo ""
done