cmake_minimum_required(VERSION 3.14)
project(ConsumerService)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka++)
pkg_check_modules(HIREDIS REQUIRED hiredis)
pkg_check_modules(YAMLCPP REQUIRED yaml-cpp)

# Source files
set(CONSUMER_SERVICE_SOURCES
    src/consumer.cpp
)

set(CONSUMER_SERVICE_HEADERS
    include/consumer.h
)

# Main executable
add_executable(consumer_service
    src/main.cpp
    ${CONSUMER_SERVICE_SOURCES}
)

target_include_directories(consumer_service
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${RDKAFKA_INCLUDE_DIRS}
        ${HIREDIS_INCLUDE_DIRS}
        ${YAMLCPP_INCLUDE_DIRS}
)

target_link_libraries(consumer_service
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        ${RDKAFKA_LIBRARIES}
        ${HIREDIS_LIBRARIES}
        ${YAMLCPP_LIBRARIES}
        Threads::Threads
)

# Tests
add_executable(consumer_service_tests
    tests/consumer_test.cpp
    ${CONSUMER_SERVICE_SOURCES}
)

target_include_directories(consumer_service_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${RDKAFKA_INCLUDE_DIRS}
        ${HIREDIS_INCLUDE_DIRS}
        ${YAMLCPP_INCLUDE_DIRS}
)

target_link_libraries(consumer_service_tests
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
        ${RDKAFKA_LIBRARIES}
        ${HIREDIS_LIBRARIES}
        ${YAMLCPP_LIBRARIES}
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

# Register tests
add_test(NAME ConsumerServiceTests COMMAND consumer_service_tests)

# Install targets
install(TARGETS consumer_service
    RUNTIME DESTINATION bin
)

install(FILES config.yml
    DESTINATION etc/atlas
)
