#include "shm.hh"
#include "args.hh"
#include "signals.hh"

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <vector>

int main(int argc, char *argv[])
{
    std::cout << "Launching client" << std::endl;
    try
    {
        Args args = parse_args(argc, argv);
        ShmManager shm_s2c(args, SHM_NAME_S2C);
        shm_s2c.init_shm();
        ShmManager shm_c2s(args, SHM_NAME_C2S);
        shm_c2s.init_shm();

        SignalManager signal_manager(SignalManager::SignalTarget::CLIENT);

        // Precompute all messages to avoid overhead during the benchmark.
        std::vector<std::string> messages;
        messages.reserve(args.iterations);
        for (ull i = 0; i < args.iterations; i++)
        {
            std::string number = std::to_string(i);
            // Create a string of the desired size filled with spaces
            std::string message(args.message_size, '.');
            std::copy(number.begin(), number.end(), message.begin());
            messages.push_back(message);
        }

        // Indicate to server client is ready
        signal_manager.notify();
        for (ull i = 0; i < args.iterations; i++)
        {
            std::string_view message = messages[i];
            // std::cout << "Client iteration i: " << i << std::endl;
            while (!shm_s2c.read_shm_until(message))
            {
                // Wait until server has written message
                // std::cout << "Waiting for server to write message" << std::endl;
            }
            shm_c2s.write_shm(message);
        }
    }
    catch (const std::exception &e)
    {
        // Catch exceptions to allow for graceful exit and stack unwinding to
        // run destructors.
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}