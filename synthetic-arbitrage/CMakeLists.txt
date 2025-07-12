cmake_minimum_required(VERSION 3.25)
project(SyntheticArbitrageEngine LANGUAGES CXX)

#  Modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#  Source files
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

#  Executable
add_executable(SyntheticArbitrage ${SOURCES})

#  Include directories
target_include_directories(SyntheticArbitrage PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/libs
    ${PROJECT_SOURCE_DIR}/libs/asio
    ${PROJECT_SOURCE_DIR}/libs/nlohmann
)

#  Link OpenSSL (for TLS WebSocket connections)
find_package(OpenSSL REQUIRED)

#  System + network libraries
find_package(Threads REQUIRED)

target_link_libraries(SyntheticArbitrage PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    ws2_32  # Windows Sockets API
)

#  Warning suppressions
target_compile_options(SyntheticArbitrage PRIVATE
    -Wno-template-id-cdtor
    -Wno-unused-parameter
    -Wno-deprecated-declarations
    -Wno-non-template-friend
)

#  Output path (optional)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)