#include "bench.hh"
#include <iostream>
#include <string>
#include <vector>

const ull NS_PER_SEC = 1000000000;

// Get the current time of the monotonically increasing clock in nanoseconds
ull get_time_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

Benchmarks::Benchmarks(const std::string &name, ull message_size) : name(name), start_ns(0), total_duration_ns(0), total_messages(0), message_size(message_size), niterations(0) {}

// Start a new benchmark iteration
// Returns -1 on error, 0 otherwise.
int Benchmarks::start_iteration()
{
    this->start_ns = get_time_ns();
    return 0;
}

// Function to add a new benchmark iteration
// Returns -1 on error, 0 otherwise.
int Benchmarks::end_iteration(ull num_messages)
{
    ull end_ns = get_time_ns();
    // End >= start, this should always be true since the clock is monotonically increasing
    if (end_ns < start_ns)
    {
        std::cerr << "End time is less than start time" << std::endl;
        return -1;
    }

    ull duration_ns = end_ns - start_ns;
    durations.push_back(duration_ns);
    total_duration_ns += duration_ns;
    total_messages += num_messages;
    ++niterations;

    return 0;
}

// Display summary statistics to stdout in a pretty printed format
void Benchmarks::report()
{
    std::cout << "========================================" << std::endl;
    std::cout << "Benchmark: " << name << " (" << message_size << " byte msgs)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Iterations: " << niterations << std::endl;
    std::cout << "Total duration (sec): " << total_duration_ns / NS_PER_SEC << std::endl;
    std::cout << "Duration (ns) / it: " << total_duration_ns / niterations << std::endl;
    std::cout << "Total messages: " << total_messages << std::endl;
    std::cout << "Messages / sec: " << total_messages * NS_PER_SEC / total_duration_ns << std::endl;
    std::cout << "Bytes / sec: " << total_messages * message_size * NS_PER_SEC / total_duration_ns << std::endl;
}

// Add a new benchmark iteration to the given Benchmarks object as a member function
