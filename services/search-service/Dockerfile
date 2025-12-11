FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY include/ ./include/
COPY src/ ./src/
COPY CMakeLists.txt ./

# Create a minimal root CMakeLists.txt for standalone build
RUN echo 'cmake_minimum_required(VERSION 3.14)' > /tmp/root_cmake.txt && \
    echo 'project(SearchService)' >> /tmp/root_cmake.txt && \
    echo 'set(CMAKE_CXX_STANDARD 17)' >> /tmp/root_cmake.txt && \
    echo 'set(CMAKE_CXX_STANDARD_REQUIRED ON)' >> /tmp/root_cmake.txt && \
    echo 'find_package(CURL REQUIRED)' >> /tmp/root_cmake.txt && \
    echo 'find_package(Threads REQUIRED)' >> /tmp/root_cmake.txt && \
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
    make search_service -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binary from builder
COPY --from=builder /app/build/search_service .

EXPOSE 8080

CMD ["./search_service"]
