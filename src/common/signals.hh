#pragma once

#include "utils.hh"
#include <csignal>
#include <iostream>
#include <unistd.h>

extern volatile sig_atomic_t server_signal_status;
extern volatile sig_atomic_t client_signal_status;

void ignore_signal(int sig);
void handle_client_signal(int sig);
void handle_server_signal(int sig);
class SignalManager
{
private:
    // Belongs to the class SignalManager, constant value
    static constexpr int SERVER_SIGNAL = SIGUSR1;
    static constexpr int CLIENT_SIGNAL = SIGUSR2;

public:
    // Benchmarks are composed of a launcher, which starts the server and client processes
    enum class SignalTarget
    {
        LAUNCHER,
        SERVER,
        CLIENT,
    };

    SignalTarget target;

    SignalManager(SignalTarget target);

    // Sends a signal with a type of the other `target` to all processes in the current process group
    // Other i.e. servers send to clients and vice versa
    void notify();

    // Pauses the current thread until a signal on `this.target` is received
    void wait_until_notify();
};
