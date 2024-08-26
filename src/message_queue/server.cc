#include "signals.hh"
#include "bench.hh"
#include "message.hh"
#include "mq.hh"
#include "args.hh"
#include "utils.hh"

#include <iostream>
// Prefer System V message queues over POSIX message queues on MacOS
// https://stackoverflow.com/questions/10079403/cannot-find-include-file-mqueue-h-on-os-x
#include <sys/msg.h>
#include <cstdio>
#include <cassert>

// Server initiates the first message, and then repeatedly ping-pongs the message with the client
void ping_pong(key_t msq_id_server_client, key_t msq_id_client_server, ull iterations, ull message_size)
{
    SignalManager signal_manager(SignalManager::SignalTarget::SERVER);
    Benchmarks benchmarks(std::string("message_queue"), message_size);
    MsgbufRAII msg_buf(message_size, SERVER_TYPE);

    // Wait until client has joined
    signal_manager.wait_until_notify();
    for (ull i = 0; i < iterations; i++)
    {
        benchmarks.start_iteration();
        // Send the message to the client
        // std::cout << "Sending message " << i << " to client" << std::endl;
        ASSERT(msg_buf.get_len() == message_size);
        // IPC_NOWAIT - return immediately if the message cannot be sent (e.g. queue full)
        // Default queue size is 16KB on Linux: https://linux.die.net/man/2/msgsnd (see `MSGMNB`)
        // On MacOS, the default appears to be 2KB
        msg_buf.data_ptr()->mtype = SERVER_TYPE;
        if (msgsnd(msq_id_server_client, msg_buf.data_ptr(), message_size, 0) == -1)
        {
            std::cerr << "Error Number: " << errno << "\n";
            report_and_exit("msgsnd");
        }

        // Receive a message from the client
        // ```
        // ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,
        //                int msgflg);
        // ```
        // Arguments:
        // 4. msgtyp: means the first message of type `msgtyp` in the queue is received
        //    unless the type is 0, in which case the first message in the queue is received
        // This call is blocking:
        // "*  If (msgflg & IPC_NOWAIT) is 0, the calling thread shall
        // suspend execution until one of the following occurs:
        // --  A message of the desired type is placed on the queue.:
        // https://man7.org/linux/man-pages/man3/msgrcv.3p.html
        if (msgrcv(msq_id_client_server, msg_buf.data_ptr(), msg_buf.get_len(), 0, 0) == -1)
        {
            report_and_exit("msgrcv");
        }
        // Benchmark one ping pong iteration
        benchmarks.end_iteration(1);
    }
}

int main(int argc, char *argv[])
{
    Args args = parse_args(argc, argv);

    if (args.iterations == 0 || args.message_size > MAX_MSG_SIZE)
    {
        std::cerr << "Iterations must be a positive integer and message_size must be less than "
                  << MAX_MSG_SIZE << "\n";
        return 1;
    }

    std::cout << "Message size: " << args.message_size << " bytes\n"
              << "Iterations: " << args.iterations << "\n";

    int msq_id_server_client = create_mq(MSG_FILE_SERVER_CLIENT);
    int msq_id_client_server = create_mq(MSG_FILE_CLIENT_SERVER);

    ping_pong(msq_id_server_client, msq_id_client_server, args.iterations, args.message_size);

    return 0;
}