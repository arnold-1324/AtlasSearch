FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source files
COPY include/ ./include/
COPY src/ ./src/
COPY CMakeLists.txt ./

# Create minimal root CMake
RUN echo 'cmake_minimum_required(VERSION 3.14)' > /tmp/root_cmake.txt && \
    echo 'project(IngestDemo)' >> /tmp/root_cmake.txt && \
    echo 'set(CMAKE_CXX_STANDARD 17)' >> /tmp/root_cmake.txt && \
    echo 'set(CMAKE_CXX_STANDARD_REQUIRED ON)' >> /tmp/root_cmake.txt && \
    echo 'find_package(CURL REQUIRED)' >> /tmp/root_cmake.txt && \
    echo 'find_package(Threads REQUIRED)' >> /tmp/root_cmake.txt && \
    echo 'find_package(Boost REQUIRED COMPONENTS system filesystem)' >> /tmp/root_cmake.txt && \
    echo 'include(FetchContent)' >> /tmp/root_cmake.txt && \
    echo 'FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.2)' >> /tmp/root_cmake.txt && \
    echo 'FetchContent_MakeAvailable(json)' >> /tmp/root_cmake.txt && \
    echo 'FetchContent_Declare(httplib GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git GIT_TAG v0.14.0)' >> /tmp/root_cmake.txt && \
    echo 'FetchContent_MakeAvailable(httplib)' >> /tmp/root_cmake.txt && \
    echo 'add_subdirectory(.)' >> /tmp/root_cmake.txt && \
    mv /tmp/root_cmake.txt /app/root_CMakeLists.txt

# Build
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make ingest_demo producer_tool -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libcurl4 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/ingest_demo .
COPY --from=builder /app/build/producer_tool .

EXPOSE 8081

CMD ["./ingest_demo"]
