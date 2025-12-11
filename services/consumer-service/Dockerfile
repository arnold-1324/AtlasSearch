FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    librdkafka-dev \
    libhiredis-dev \
    libyaml-cpp-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source files
COPY include/ ./include/
COPY src/ ./src/
COPY CMakeLists.txt ./
COPY config.yml ./

# Create minimal root CMake
RUN echo 'cmake_minimum_required(VERSION 3.14)' > /tmp/root_cmake.txt && \
    echo 'project(ConsumerService)' >> /tmp/root_cmake.txt && \
    echo 'set(CMAKE_CXX_STANDARD 17)' >> /tmp/root_cmake.txt && \
    echo 'set(CMAKE_CXX_STANDARD_REQUIRED ON)' >> /tmp/root_cmake.txt && \
    echo 'find_package(CURL REQUIRED)' >> /tmp/root_cmake.txt && \
    echo 'find_package(Threads REQUIRED)' >> /tmp/root_cmake.txt && \
    echo 'find_package(PkgConfig REQUIRED)' >> /tmp/root_cmake.txt && \
    echo 'pkg_check_modules(RDKAFKA REQUIRED rdkafka++)' >> /tmp/root_cmake.txt && \
    echo 'pkg_check_modules(HIREDIS REQUIRED hiredis)' >> /tmp/root_cmake.txt && \
    echo 'pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)' >> /tmp/root_cmake.txt && \
    echo 'include(FetchContent)' >> /tmp/root_cmake.txt && \
    echo 'FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.2)' >> /tmp/root_cmake.txt && \
    echo 'FetchContent_MakeAvailable(json)' >> /tmp/root_cmake.txt && \
    echo 'add_subdirectory(.)' >> /tmp/root_cmake.txt && \
    mv /tmp/root_cmake.txt /app/root_CMakeLists.txt

# Build
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make consumer_service -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libcurl4 \
    librdkafka++1 \
    libhiredis0.14 \
    libyaml-cpp0.7 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/consumer_service .
COPY --from=builder /app/config.yml .

CMD ["./consumer_service", "config.yml"]
