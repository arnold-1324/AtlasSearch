cmake_minimum_required(VERSION 3.14)
project(IngestDemo)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system filesystem)

# Source files
set(INGEST_DEMO_SOURCES
    src/ingest_server.cpp
)

set(INGEST_DEMO_HEADERS
    include/ingest_server.h
)

# Main executable
add_executable(ingest_demo
    src/main.cpp
    ${INGEST_DEMO_SOURCES}
)

target_include_directories(ingest_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(ingest_demo
    PRIVATE
        nlohmann_json::nlohmann_json
        httplib::httplib
        Threads::Threads
        ${Boost_LIBRARIES}
        stdc++fs  # For std::filesystem on older compilers
)

# Producer tool
add_executable(producer_tool
    src/producer_tool.cpp
)

target_link_libraries(producer_tool
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Tests
add_executable(ingest_demo_tests
    tests/ingest_test.cpp
    ${INGEST_DEMO_SOURCES}
)

target_include_directories(ingest_demo_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(ingest_demo_tests
    PRIVATE
        nlohmann_json::nlohmann_json
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
        ${Boost_LIBRARIES}
        stdc++fs
)

# Register tests
add_test(NAME IngestDemoTests COMMAND ingest_demo_tests)

# Install targets
install(TARGETS ingest_demo producer_tool
    RUNTIME DESTINATION bin
)
