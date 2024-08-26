#include "args.hh"
#include "utils.hh"
#include "signals.hh"
#include "bench.hh"

#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <vector>
#include <unistd.h>

constexpr const char *SOCKET_PATH = "/tmp/cpp_ipc_benchmarks";

int main(int argc, char *argv[])
{
    std::cout << "Launching client" << std::endl;
    Args args = parse_args(argc, argv);
    SignalManager signal_manager(SignalManager::SignalTarget::SERVER);
    Benchmarks benchmarks(std::string("shm"), args.message_size);

    int server_fd, client_fd;
    struct sockaddr_un addr;

    // Create the socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        report_and_exit("socket");
    }

    // Clear and set up the address structure
    memset(&addr, 0, sizeof(addr));
    // "The sun_family field always contains AF_UNIX"
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Bind the socket to the address
    unlink(SOCKET_PATH); // Remove any existing socket file
    // assigns the address specified by addr to the socket referred to by the file descriptor sockfd
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        close(server_fd);
        report_and_exit("bind");
    }

    // Listen for incoming connections
    int max_connections = 1;
    if (listen(server_fd, max_connections) == -1)
    {
        close(server_fd);
        report_and_exit("listen");
    }

    std::cout << "Server listening on " << SOCKET_PATH << std::endl;

    // Accept a connection
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1)
    {
        close(server_fd);
        report_and_exit("accept");
    }

    // Wait until client notifies that it is ready
    signal_manager.wait_until_notify();
    std::vector<char> buffer(args.message_size, 'a');
    for (int i = 0; i < args.iterations; i++)
    {
        benchmarks.start_iteration();
        int bytes_received = read(client_fd, buffer.data(), buffer.size());
        ASSERT(bytes_received == buffer.size());
        // Assert that buffer is all 'a's
        for (int j = 0; j < buffer.size(); j++)
        {
            ASSERT(buffer[j] == 'a');
        }

        write(client_fd, buffer.data(), buffer.size());
        benchmarks.end_iteration(1);
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH); // Clean up the socket file
    return 0;
}