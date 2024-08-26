#pragma once

#include "types.hh"
#include "args.hh"

#include <string>

// Cannot have additional slashes in the name
constexpr std::string_view SHM_NAME_S2C = "/koi_shm_bench_s2c_v8";
constexpr std::string_view SHM_NAME_C2S = "/koi_shm_bench_c2s_v8";

// shm holds at most `SHM_NUM_MSG` messages, 2^12 = 4096
constexpr unsigned int SHM_NUM_MSG = 1 << 12;

class ShmManager
{
    char *shm_ptr;
    off_t write_offset;
    off_t read_offset;
    size_t shm_size;
    size_t message_size;
    const std::string_view shm_name;

public:
    // Creates `ShmManager` and preallocates a `read_buffer` of size `message_size`.
    ShmManager(const Args &args, const std::string_view shm_name);
    ~ShmManager();
    // This must be called before any other operation to initialize the shared memory.
    void init_shm();
    // Get size of the shm, in bytes
    size_t get_shm_size() const;
    // This operation is unchecked. The message will be appended to the
    // shared memory at the current position. Users should ensure that the
    // shared memory has enough remaining space to hold the message.
    void write_shm(const std::string_view message);
    // Reads a message of size `message_sz` from the shared memory of size `message_sz`, returns
    // if the read message is equal to the `target`. If so, it advances the internal read pointer.
    bool read_shm_until(std::string_view target);
    // Reads range of all bytes written to shared memory and returns as a string view.
    std::string_view read_all_shm();
    // Returns the number of bytes written to the shared memory.
    size_t get_write_offset() const;
};