# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(CacheEffects LANGUAGES CXX)

# Set the mandatory C++ standard
# e.g. `std::format` is since C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Static library (.a files)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Executable files

# Ensure output folders exist
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/message_queue)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/named_pipe)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pipe)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shm)

# Source files in src/common directory
set(COMMON_SOURCES
    src/common/args.cc
    src/common/bench.cc
    src/common/launcher.cc
    src/common/signals.cc
    src/common/utils.cc
)

# Create a static or shared library from the common sources
add_library(common_lib STATIC ${COMMON_SOURCES})

# Include directories for common headers (if needed)
target_include_directories(common_lib PUBLIC src/common)

# Shared Launcher (for Message Queue, Named Pipe, and shm)
add_executable(launcher src/common/launcher.cc)

target_include_directories(launcher PUBLIC src/common)
target_link_libraries(launcher PRIVATE common_lib)

# IPC Methods
# Message Queue
add_executable(message_queue_client src/message_queue/client.cc)
add_executable(message_queue_server src/message_queue/server.cc)
add_executable(message_queue_ops src/message_queue/queue_ops.cc)

target_link_libraries(message_queue_client PRIVATE common_lib)
target_link_libraries(message_queue_server PRIVATE common_lib)
target_link_libraries(message_queue_ops PRIVATE common_lib)

set_target_properties(message_queue_client PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/message_queue
    OUTPUT_NAME "client")
set_target_properties(message_queue_server PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/message_queue
    OUTPUT_NAME "server")
set_target_properties(message_queue_ops PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/message_queue
    OUTPUT_NAME "queue_ops")

# Named Pipe
add_executable(named_pipe_client src/named_pipe/client.cc)
add_executable(named_pipe_server src/named_pipe/server.cc)

add_library(named_pipe_common STATIC src/named_pipe/named_pipe.cc)
target_include_directories(named_pipe_common PUBLIC src/named_pipe src/common)

target_link_libraries(named_pipe_client PRIVATE common_lib named_pipe_common)
target_link_libraries(named_pipe_server PRIVATE common_lib named_pipe_common)

set_target_properties(named_pipe_client PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/named_pipe
    OUTPUT_NAME "client")
set_target_properties(named_pipe_server PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/named_pipe
    OUTPUT_NAME "server")

# SHM
add_executable(shm_client src/shm/client.cc)
add_executable(shm_server src/shm/server.cc)

add_library(shm_common STATIC src/shm/shm.cc)
target_include_directories(shm_common PUBLIC src/shm src/common)

target_link_libraries(shm_client PRIVATE common_lib shm_common)
target_link_libraries(shm_server PRIVATE common_lib shm_common)

set_target_properties(shm_client PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shm
    OUTPUT_NAME "client")
set_target_properties(shm_server PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shm
    OUTPUT_NAME "server")

# Unnamed Pipe
add_executable(pipe src/pipe/pipe.cc)

target_include_directories(pipe PUBLIC src/common)

target_link_libraries(pipe PRIVATE common_lib)

set_target_properties(pipe PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/pipe
    OUTPUT_NAME "pipe")

# Unix Socket
add_executable(unix_socket_client src/unix_socket/client.cc)
add_executable(unix_socket_server src/unix_socket/server.cc)

target_include_directories(unix_socket_client PUBLIC src/common)
target_include_directories(unix_socket_server PUBLIC src/common)

target_link_libraries(unix_socket_client PRIVATE common_lib)
target_link_libraries(unix_socket_server PRIVATE common_lib)

set_target_properties(unix_socket_client PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unix_socket
    OUTPUT_NAME "client")
set_target_properties(unix_socket_server PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/unix_socket
    OUTPUT_NAME "server")