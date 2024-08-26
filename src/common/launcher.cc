#include "args.hh"
#include "utils.hh"
#include "signals.hh"

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <format>
#include <signal.h>

int main(int argc, char *argv[])
{
    LauncherArgs args = parse_launcher_args(argc, argv);

    // Convert integers to strings
    std::string message_size_str = std::to_string(args.message_size);
    std::string iterations_str = std::to_string(args.iterations);

    // Convert strings to const char*
    const char *message_size = message_size_str.c_str();
    const char *iterations = iterations_str.c_str();

    std::cout << "Message size and iterations: " << message_size << " " << iterations << std::endl;

    // Registers signal handlers for the launcher which ignores all user signals (SIGUSR1, SIGUSR2)
    // which would otherwise terminate the launcher process
    SignalManager signal_manager = SignalManager(SignalManager::SignalTarget::LAUNCHER);
    (void)signal_manager;

    // Fork to create the server process
    pid_t server_pid = fork();
    if (server_pid < 0)
    {
        std::cerr << "Failed to fork server process" << std::endl;
        report_and_exit("fork");
        return 1;
    }
    else if (server_pid == 0)
    {
        std::string server_binary = std::format("bin/{}/server", args.benchmark_name);
        // Child process for server
        execl(server_binary.c_str(), "server", "-m", message_size, "-i", iterations, (char *)NULL);
        // If execl returns, it means there was an error
        std::cerr << "Failed to execute server process" << std::endl;
        report_and_exit("execl");
    }

    // Sleep for a bit to allow the server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Fork to create the child process
    pid_t client_pid = fork();
    if (client_pid < 0)
    {
        std::cerr << "Failed to fork client process" << std::endl;
        report_and_exit("fork");
    }
    else if (client_pid == 0)
    {
        std::string client_binary = std::format("bin/{}/client", args.benchmark_name);
        std::cout << "Client binary: " << client_binary << std::endl;
        // Client process
        execl(client_binary.c_str(), "client", "-m", message_size, "-i", iterations, (char *)NULL);
        // If execl returns, it means there was an error
        std::cerr << "Failed to execute client process" << std::endl;
        report_and_exit("execl");
    }

    // Wait for the client and server to finish
    int status;
    if (waitpid(server_pid, &status, 0) == -1)
    {
        report_and_exit("waitpid server");
    }

    if (waitpid(client_pid, &status, 0) == -1)
    {
        report_and_exit("waitpid client");
    }

    return 0;
}