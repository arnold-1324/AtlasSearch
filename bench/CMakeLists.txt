cmake_minimum_required(VERSION 3.14)
project(BenchTool)

# Benchmark tool executable
add_executable(bench_tool
    src/bench_tool.cpp
)

target_link_libraries(bench_tool
    PRIVATE
        CURL::libcurl
        Threads::Threads
)

# Install target
install(TARGETS bench_tool
    RUNTIME DESTINATION bin
)
