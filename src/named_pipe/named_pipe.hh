#pragma once

#include "args.hh"

#include <vector>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>

const std::string server_to_client_fifo = "/tmp/server_to_client_fifo";
const std::string client_to_server_fifo = "/tmp/client_to_server_fifo";

class FifoManager
{
private:
    const std::string _fifo_path;
    const Args _args;
    int _fd;
    std::vector<char> _buf;

    // Function to create a named pipe (FIFO)
    void _create_fifo();

    // `open_fifo` must be called before `read_fifo`, otherwise `fd` will be uninitialized
    // and `read` would return `EBADF (Bad file descriptor)`
    // This class will automatically call `open_fifo` on construction, so this is not a problem.
    void _read_fifo();

    // `open_fifo` must be called before `write_fifo`, otherwise `fd` will be uninitialized
    // and `read` would return `EBADF (Bad file descriptor)`
    // This class will automatically call `open_fifo` on construction, so this is not a problem.
    void _write_fifo(const std::vector<char> &buf);

    // This must be called before `read_fifo` or `write_fifo`
    void _open_fifo(const std::string &fifo_path);

    // Initialize the buffer with a default value of 0
    void _init_buf();

public:
    FifoManager(const std::string &fifo_path, const Args &args);

    ~FifoManager();

    void write_fifo(const std::vector<char> &input_buf);

    void write_fifo();

    std::vector<char> &read_fifo();
};
