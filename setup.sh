#!/bin/bash
# Exit immediately if a command exits with a non-zero status
set -e

echo "=== Starting System Restoration ==="

# 1. Update the local package listings
sudo apt-get update

# 2. Add the toolchain PPA to guarantee access to GCC 15
sudo apt-get install -y software-properties-common
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update

# 3. Install your exact programming compilers and libraries
echo "Installing core C++ toolchain and packages..."
sudo apt-get install -y \
    build-essential \
    gcc-15 \
    g++-15 \
    make \
    libcurl4-openssl-dev \
    libcrypto++-dev \
    libgtest-dev \
    cmake \
    git

# 4. Enforce g++-15 as your default compiler alias
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 100

echo "=== Restoration Complete! System matches original machine. ==="

