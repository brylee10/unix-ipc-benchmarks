#include "shm.hh"
#include "args.hh"
#include "signals.hh"
#include "bench.hh"

#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <vector>

int main(int argc, char *argv[])
{
    try
    {
        Args args = parse_args(argc, argv);
        std::cout << "Launching server" << std::endl;
        SignalManager signal_manager(SignalManager::SignalTarget::SERVER);
        Benchmarks benchmarks(std::string("shm"), args.message_size);

        ShmManager shm_s2c(args, SHM_NAME_S2C);
        shm_s2c.init_shm();
        std::cout << "SHM size: " << shm_s2c.get_shm_size() << std::endl;
        ShmManager shm_c2s(args, SHM_NAME_C2S);
        shm_c2s.init_shm();

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

        // Wait until client notifies that it is ready
        signal_manager.wait_until_notify();
        for (ull i = 0; i < args.iterations; i++)
        {
            std::string_view message = messages[i];
            benchmarks.start_iteration();
            // std::cout << "Server iteration: " << i << std::endl;
            shm_s2c.write_shm(message);
            while (!shm_c2s.read_shm_until(message))
            {
                // Wait until client has written its message
                // std::cout << "Waiting for client to write message" << std::endl;
            }
            // Print all written shared memory
            benchmarks.end_iteration(1);
        }
        return 0;
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