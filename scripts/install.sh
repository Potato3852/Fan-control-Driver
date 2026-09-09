#!/usr/bin/env bash

set =e

echo "[1/4] Building..."
cmake -B build -DCMAKE_CXX_COMPILE=g++
cmake --build build

echo "[2/4] Install binary and systemd-unit..."
sudo cmake --install build

echo "[3/4] Restart systemd service..."
sudo systemctl daemon-reload
sudo systemctl enable --now zenbook-fan

echo "[4/4] Checking..."
sudo systemctl status zenbook-fan --no-pager

echo ""
if [ -f "scripts/zenbook-fan-notify" ]; then
    read -p "Do you want to install desktop notification helper (zenbook-fan-notify)? [y/N]: " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo cp scripts/zenbook-fan-notify /usr/local/bin/zenbook-fan-notify
        sudo chmod +x /usr/local/bin/zenbook-fan-notify
        echo "[INFO] Installed /usr/local/bin/zenbook-fan-notify successfully."
    fi
fi

echo ""
echo "Installation is complete!"