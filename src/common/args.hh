#pragma once

#include "types.hh"

#include <getopt.h>
#include <iostream>

struct Args
{
    size_t message_size;
    unsigned long long iterations;
};

struct LauncherArgs
{
    size_t message_size;
    unsigned long long iterations;
    std::string benchmark_name;
};

Args parse_args(int argc, char *argv[]);

LauncherArgs parse_launcher_args(int argc, char *argv[]);
