#include "args.hh"
#include "named_pipe.hh"
#include "bench.hh"
#include "types.hh"
#include "signals.hh"

void start_server(Args args)
{
    FifoManager fifo_s2c(server_to_client_fifo, args);
    FifoManager fifo_c2s(client_to_server_fifo, args);
    SignalManager signal_manager = SignalManager(SignalManager::SignalTarget::CLIENT);

    // Notify the client to start
    signal_manager.notify();
    for (ull i = 0; i < args.iterations; i++)
    {
        // Full ping pong
        fifo_s2c.read_fifo();
        fifo_c2s.write_fifo();
    }
}

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);
    start_server(args);

    return 0;
}