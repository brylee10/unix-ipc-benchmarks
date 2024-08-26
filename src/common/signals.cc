#include "signals.hh"
#include "utils.hh"
#include <csignal>
#include <iostream>
#include <unistd.h>

volatile sig_atomic_t server_signal_status = 0;
volatile sig_atomic_t client_signal_status = 0;

// Default action on `SIGUSR1`/`SIGUSR2` is abnormal termination of the process
// per https://pubs.opengroup.org/onlinepubs/9699919799/. Register a no-op handler for
// any otherwise unhandled signals to avoid this behavior.
void ignore_signal([[maybe_unused]] int sig)
{
}

void handle_client_signal(int sig)
{
    client_signal_status = sig;
}

void handle_server_signal(int sig)
{
    server_signal_status = sig;
}

SignalManager::SignalManager(SignalTarget target) : target(target)
{
    switch (target)
    {
    case SignalTarget::LAUNCHER:
        signal(SERVER_SIGNAL, ignore_signal);
        signal(CLIENT_SIGNAL, ignore_signal);
        break;
    case SignalTarget::SERVER:
        signal(SERVER_SIGNAL, handle_server_signal);
        signal(CLIENT_SIGNAL, ignore_signal);
        break;
    case SignalTarget::CLIENT:
        signal(CLIENT_SIGNAL, handle_client_signal);
        signal(SERVER_SIGNAL, ignore_signal);
        break;
    }
}

// Sends a signal with a type of the other `target` to all processes in the current process group
// Other i.e. servers send to clients and vice versa
void SignalManager::notify()
{
    int sig = (target == SignalTarget::SERVER) ? CLIENT_SIGNAL : SERVER_SIGNAL;
    // std::cout << "Notify process group with signal " << sig << std::endl;
    kill(0, sig);
}

// Pauses the current thread until a signal on `this.target` is received
void SignalManager::wait_until_notify()
{
    while (true)
    {
        if (target == SignalTarget::SERVER && server_signal_status == SERVER_SIGNAL)
        {
            server_signal_status = 0;
            break;
        }
        else if (target == SignalTarget::CLIENT && client_signal_status == CLIENT_SIGNAL)
        {
            client_signal_status = 0;
            break;
        }
        // Suspends execution until a signal is received
        // Commented because this leads to a deadlock. If the signal is received
        // before the thread is paused, the signal will be missed.
        // pause();
    }
}
