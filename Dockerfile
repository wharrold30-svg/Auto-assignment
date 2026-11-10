# Use a modern Ubuntu base image matching your current platform
FROM ubuntu:24.04

# Prevent interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Add the Ubuntu toolchain PPA to guarantee access to cutting-edge GCC 15 packages
RUN apt-get update && apt-get install -y software-properties-common \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test

# Install core build tools, C++23/26 capable GCC-15, and your dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-15 \
    g++-15 \
    make \
    libcurl4-openssl-dev \
    libcrypto++-dev \
    libgtest-dev \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Set GCC-15 as the default system compiler inside the container
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 100

# Set the working directory inside the container
WORKDIR /workspace

