#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "types.hh"

extern const ull NS_PER_SEC;

class Benchmarks
{
private:
    // Identifier for the benchmark
    const std::string name;
    // Start of most recent iteration (ns)
    ull start_ns;
    // Vector duration of each iteration (ns)
    std::vector<ull> durations;
    // Total duration of all iterations (ns)
    ull total_duration_ns = 0;
    // Total number of messages sent
    ull total_messages = 0;
    // Message size
    ull message_size;
    // Number of iterations
    ull niterations;

public:
    // Const lvalue reference allows rvalues in constructor
    Benchmarks(const std::string &name, ull message_size);

    // Internally sets `start_ns` to the current time in nano seconds as measured
    // by the system monotonically increasing clock
    int start_iteration();

    // Function to add a new benchmark iteration
    // Internally calculates the duration since the last call to `start_iteration`
    int end_iteration(ull num_messages);

    // Display summary statistics to stdout in a pretty printed format
    void report();

    // Display the report in the destructor so statistics are always printed
    ~Benchmarks()
    {
        report();
    }
};

// Add a new benchmark iteration to the given Benchmarks object as a member function
