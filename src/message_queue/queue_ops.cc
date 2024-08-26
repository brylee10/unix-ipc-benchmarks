#include "mq.hh"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

void delete_message_queue(int msq_id)
{
    if (msgctl(msq_id, IPC_RMID, nullptr) == -1)
    {
        std::cerr << "Error Number: " << errno << std::endl;
        perror("msgctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << "Message queue " << msq_id << " deleted successfully.\n";
    }
}

void increase_queue_capacity(int msq_id, size_t new_capacity)
{
    struct msqid_ds buf;

    if (msgctl(msq_id, IPC_STAT, &buf) == -1)
    {
        perror("msgctl IPC_STAT");
        exit(EXIT_FAILURE);
    }

    buf.msg_qbytes = new_capacity;

    if (msgctl(msq_id, IPC_SET, &buf) == -1)
    {
        perror("msgctl IPC_SET");
        exit(EXIT_FAILURE);
    }

    struct msqid_ds buf2;
    if (msgctl(msq_id, IPC_STAT, &buf2) == -1)
    {
        perror("msgctl IPC_STAT");
        exit(EXIT_FAILURE);
    }

    if (buf2.msg_qbytes != new_capacity)
    {
        std::cerr << "Failed to increase queue capacity to " << new_capacity << " bytes."
                  << "This may be beyond the maximum queue size. Try a lower capacity. "
                  << " Current capacity is " << buf2.msg_qbytes << " bytes." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Queue capacity increased to " << new_capacity << " bytes.\n";
}

void get_info(int msq_id)
{
    struct msqid_ds buf;

    if (msgctl(msq_id, IPC_STAT, &buf) == -1)
    {
        perror("msgctl IPC_STAT");
        exit(EXIT_FAILURE);
    }

    std::cout << "Message queue information:\n";
    std::cout << "  Message queue identifier: " << msq_id << "\n";
    std::cout << "  Message queue uid: " << buf.msg_perm.uid << "\n";
    std::cout << "  Message queue gid: " << buf.msg_perm.gid << "\n";
    std::cout << "  Message queue last send: " << ctime(&buf.msg_stime);
    std::cout << "  Message queue last recv: " << ctime(&buf.msg_rtime);
    std::cout << "  Message queue ctime: " << ctime(&buf.msg_ctime);
    std::cout << "  Message queue number of bytes: " << buf.msg_cbytes << "\n";
    std::cout << "  Message queue qnum: " << buf.msg_qnum << "\n";
    std::cout << "  Message queue qbytes: " << buf.msg_qbytes << "\n";
    std::cout << "  Message queue lspid: " << buf.msg_lspid << "\n";
    std::cout << "  Message queue lrpid: " << buf.msg_lrpid << "\n";
}

int main(int argc, char *argv[])
{
    // When running `expand`, `sudo` may be needed
    // ```
    // sudo bin/message_queue/queue_ops expand 32000
    // ```
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <action> <target> [<value>]\n";
        std::cerr << "Actions:\n";
        std::cerr << "  delete         Delete the message queue\n";
        std::cerr << "  expand <size>  Expand the queue capacity to <size> bytes\n";
        std::cerr << "  info           Print information about the message queue\n";
        std::cerr << "Targets:\n";
        std::cerr << "  server         Server message queue\n";
        std::cerr << "  client         Client message queue\n";
        return 1;
    }

    std::string action = argv[1];
    std::string target = argv[2];
    const char *msg_file = nullptr;

    if (target == "server")
    {
        msg_file = MSG_FILE_SERVER_CLIENT;
    }
    else if (target == "client")
    {
        msg_file = MSG_FILE_CLIENT_SERVER;
    }
    else
    {
        std::cerr << "Unknown target: " << target << "\n";
        return 1;
    }

    int msq_id = create_mq(msg_file);

    if (action == "delete")
    {
        delete_message_queue(msq_id);
    }
    else if (action == "expand")
    {
        if (argc != 4)
        {
            std::cerr << "Usage: " << argv[0] << " expand <size>\n";
            return 1;
        }
        size_t new_capacity = std::stoul(argv[2]);
        increase_queue_capacity(msq_id, new_capacity);
    }
    else if (action == "info")
    {
        get_info(msq_id);
    }
    else
    {
        std::cerr << "Unknown action: " << action << "\n";
        return 1;
    }

    return 0;
}