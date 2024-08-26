#include "named_pipe.hh"

void FifoManager::_create_fifo()
{
    if (mkfifo(_fifo_path.c_str(), 0666) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }
}

void FifoManager::_open_fifo(const std::string &fifo_path)
{
    _fd = open(fifo_path.c_str(), O_RDWR); // O_RDWR to allow both read and write
    if (_fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
}

void FifoManager::_read_fifo()
{
    // std::cout << "Reading from FIFO" << std::endl;
    ssize_t bytes_read = read(_fd, _buf.data(), _buf.size());
    if (bytes_read == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    // std::cout << "Read " << bytes_read << " bytes: " << std::string(_buf.begin(), _buf.end()) << std::endl;
}

void FifoManager::_write_fifo(const std::vector<char> &buf)
{
    // std::cout << "Writing to FIFO" << std::endl;
    ssize_t bytes_written = write(_fd, buf.data(), buf.size());
    if (bytes_written == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
    // std::cout << "Wrote " << bytes_written << " bytes: " << std::string(buf.begin(), buf.end()) << std::endl;
}

void FifoManager::_init_buf()
{
    // Initialize the buffer with a default value of 0
    _buf = std::vector<char>(_args.message_size, 0);
}

FifoManager::FifoManager(const std::string &fifo_path, const Args &args)
    : _fifo_path(fifo_path), _args(args), _fd(-1)
{
    _create_fifo();
    _open_fifo(_fifo_path);
    _init_buf();
}

FifoManager::~FifoManager()
{
    if (_fd != -1)
    {
        close(_fd);
    }

    if (unlink(_fifo_path.c_str()) == -1)
    {
        // The client and server will both try to unlink the FIFO, so the file
        // may not exist.
        if (errno != ENOENT)
        {
            perror("unlink");
            exit(EXIT_FAILURE);
        }
    }
}

void FifoManager::write_fifo(const std::vector<char> &input_buf)
{
    _write_fifo(input_buf);
}

void FifoManager::write_fifo()
{
    _write_fifo(_buf);
}

std::vector<char> &FifoManager::read_fifo()
{
    _read_fifo();
    return _buf;
}
