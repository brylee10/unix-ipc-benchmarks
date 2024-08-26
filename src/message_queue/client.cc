#include "message.hh"
#include "mq.hh"
#include "args.hh"
#include "signals.hh"

#include <cassert>
#include <sys/msg.h>

// Reads a message from the message queue and writes it back to the server
void ping_pong(key_t msq_id_server_client, key_t msq_id_client_server, ull iterations, ull message_size)
{
    SignalManager signal_manager(SignalManager::SignalTarget::CLIENT);
    MsgbufRAII msg_buf(message_size, CLIENT_TYPE);

    // Notify server that client has joined
    signal_manager.notify();
    for (ull i = 0; i < iterations; i++)
    {
        // Send the message to the client
        // std::cout << "Receiving message " << i << " from server\n";
        assert(msg_buf.get_len() == message_size);
        // Receive a message from the client
        // ```
        // ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,
        //                int msgflg);
        // ```
        // Arguments:
        // 4. msgtyp: means the first message of type `msgtyp` in the queue is received
        //    unless the type is 0, in which case the first message in the queue is received
        if (msgrcv(msq_id_server_client, msg_buf.data_ptr(), msg_buf.get_len(), 0, 0) == -1)
        {
            std::cerr << "Error Number: " << errno << std::endl;
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        msg_buf.data_ptr()->mtype = CLIENT_TYPE;
        // std::cout << "Sending message " << i << " to server" << std::endl;
        if (msgsnd(msq_id_client_server, msg_buf.data_ptr(), msg_buf.get_len(), 0) == -1)
        {
            std::cerr << "Error Number: " << errno << "\n";
            perror("msgsnd");
            return;
        }
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