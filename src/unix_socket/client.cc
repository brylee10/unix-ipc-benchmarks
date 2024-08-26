#include "args.hh"
#include "utils.hh"
#include "signals.hh"

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
    SignalManager signal_manager(SignalManager::SignalTarget::CLIENT);

    int client_fd;
    struct sockaddr_un addr;

    // Create the socket
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        report_and_exit("socket");
    }

    // Clear and set up the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        close(client_fd);
        report_and_exit("connect");
    }

    // Indicate to server client is ready
    signal_manager.notify();
    std::vector<char> buffer(args.message_size, 'a');
    for (int i = 0; i < args.iterations; i++)
    {
        // Write the message to the server
        int bytes_sent = write(client_fd, buffer.data(), buffer.size());
        ASSERT(bytes_sent == buffer.size());

        // Read the response from the server
        int bytes_received = read(client_fd, buffer.data(), buffer.size());
        ASSERT(bytes_received == buffer.size());

        // Assert that the buffer is still all 'a's
        for (int j = 0; j < buffer.size(); j++)
        {
            ASSERT(buffer[j] == 'a');
        }
    }

    close(client_fd);
    return 0;
}
