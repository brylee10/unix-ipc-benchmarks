#pragma once

#include "utils.hh"
#include "types.hh"

#include <iostream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

constexpr ull MAX_MSG_SIZE = 8192; // Maximum message size for POSIX queues
// Uses two queues, one for each direction. Alternatively, a single queue could be used
// with a message type to differentiate between server and client messages.
// This approach gives more flexibility, since `create_mq` can be called with the same
// `msg_file` for both directions and be equal to the special case of one message queue.
// Server -> Client
constexpr const char *MSG_FILE_SERVER_CLIENT = "/tmp/mq_server_client";
// Client -> Server
constexpr const char *MSG_FILE_CLIENT_SERVER = "/tmp/mq_client_server";

int create_mq_dir(const char *msg_file)
{
    // Check if directory exists (optional)
    struct stat st;
    if (stat(msg_file, &st) == 0 && S_ISDIR(st.st_mode))
    {
        // Directory already exists, continue
        return 0;
    }

    std::cout << "Creating message queue directory: " << msg_file << std::endl;

    // Create directory with permissions 0666
    if (mkdir(msg_file, 0666) == -1)
    {
        report_and_exit("mkdir");
    }

    return 0;
}

// Creates a System V message queue, returning the message queue identifier
int create_mq(const char *msg_file)
{
    // Make the message file if it doesn't already exist
    create_mq_dir(msg_file);

    key_t mq_key;

    // Generates a deterministic key for a System V message queue
    // Assumes program is run from the root of the repository, so `benchmarks/message_queue`
    // will exist
    mq_key = ftok(msg_file, 'X');
    if (mq_key == -1)
    {
        report_and_exit("ftok");
    }

    // Per: https://pubs.opengroup.org/onlinepubs/9699919799/functions/msgget.html
    // The low-order 9 bits of msg_perm.mode shall be set to the low-order 9 bits of msgflg.
    int msq_id = msgget(mq_key, IPC_CREAT | 0666);

    if (msq_id == -1)
    {
        report_and_exit("msgget");
    }

    return msq_id;
}