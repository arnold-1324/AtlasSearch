cmake_minimum_required(VERSION 3.14)
project(SearchService)

# Source files
set(SEARCH_SERVICE_SOURCES
    src/search_service.cpp
)

set(SEARCH_SERVICE_HEADERS
    include/search_service.h
)

# Main executable
add_executable(search_service
    src/main.cpp
    ${SEARCH_SERVICE_SOURCES}
)

target_include_directories(search_service
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(search_service
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        httplib::httplib
        Threads::Threads
)

# Tests
add_executable(search_service_tests
    tests/search_service_test.cpp
    ${SEARCH_SERVICE_SOURCES}
)

target_include_directories(search_service_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(search_service_tests
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

# Register tests
add_test(NAME SearchServiceTests COMMAND search_service_tests)

# Install targets
install(TARGETS search_service
    RUNTIME DESTINATION bin
)
