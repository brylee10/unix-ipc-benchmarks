/*
 * This benchmark measures the time to send a message of a given size through a pipe.
 */

#include "utils.hh"
#include "bench.hh"
#include "signals.hh"
#include "args.hh"

#include <iostream>
#include <unistd.h>
#include <string>
#include <thread>

#define READ_FD 0
#define WRITE_FD 1

void start_child(int pipefd_s2c[2], int pipefd_c2s[2], ull message_size, ull iterations)
{
    SignalManager signal_manager(SignalManager::SignalTarget::CLIENT);

    // Child process does not write to s2c, and does not read from c2s
    close(pipefd_s2c[WRITE_FD]);
    close(pipefd_c2s[READ_FD]);

    // Notify server that client is ready to read (server will start first)
    signal_manager.notify();
    char *buffer = new char[message_size];
    for (ull i = 0; i < iterations; i++)
    {

        if (read(pipefd_s2c[READ_FD], buffer, message_size) < 0)
        {
            report_and_exit("read() failed");
        }

        if (write(pipefd_c2s[WRITE_FD], buffer, message_size) < 0)
        {
            report_and_exit("write() failed");
        }
    }

    delete[] buffer;
    close(pipefd_s2c[READ_FD]);
    close(pipefd_c2s[WRITE_FD]);
}

void start_parent(int pipefd_s2c[2], int pipefd_c2s[2], ull message_size, ull iterations)
{
    Benchmarks benchmarks(std::string("pipe"), message_size);
    SignalManager signal_manager(SignalManager::SignalTarget::SERVER);
    // Parent process does not read from s2c, and does not write to c2s
    close(pipefd_s2c[READ_FD]);
    close(pipefd_c2s[WRITE_FD]);

    char *buffer = new char[message_size];

    // Generate a string with `message_size` number of bytes
    std::string text = std::string(message_size, 'a');
    if (text.copy(buffer, message_size, 0) != message_size)
    {
        report_and_exit("copy() did not copy full message size");
    }

    // Wait for client to notify that it has joined
    signal_manager.wait_until_notify();
    for (ull i = 0; i < iterations; i++)
    {
        // Benchmarks server to client write
        benchmarks.start_iteration();
        if (write(pipefd_s2c[WRITE_FD], buffer, message_size) < 0)
        {
            report_and_exit("write() failed");
        }
        // A successful read implies the client finished its read and wrote data
        // "If a process attempts to read from an empty pipe, then read(2)
        // will block until data is available."
        // https://man7.org/linux/man-pages/man7/pipe.7.html
        if (read(pipefd_c2s[READ_FD], buffer, message_size) < 0)
        {
            report_and_exit("read() failed");
        }
        // Each iteration is 1 ping pong message
        benchmarks.end_iteration(1);
    }

    delete[] buffer;
    close(pipefd_s2c[WRITE_FD]);
    close(pipefd_c2s[READ_FD]);
}

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);

    int pipefd_s2c[2];
    if (pipe(pipefd_s2c) != 0)
    {
        report_and_exit("pipe() server to client failed");
        return 1;
    }

    int pipefd_c2s[2];
    if (pipe(pipefd_c2s) != 0)
    {
        report_and_exit("pipe() client to server failed");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        report_and_exit("fork() failed");
    }

    if (pid == 0)
    {
        // Allow time for the server to start first
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        // Child process
        start_child(pipefd_s2c, pipefd_c2s, args.message_size, args.iterations);
    }
    else
    {
        // Parent process
        start_parent(pipefd_s2c, pipefd_c2s, args.message_size, args.iterations);
    }

    return 0;
}